#! /usr/bin/env python

import subprocess
import types
import os
import sys
import getopt
import resource

# The superset of wordsizes that should be tested (default 32, 64)
wordsizes = set()

# The super set of page sizes that should be tested.  Defaults to all supported
# huge page sizes with an active mount and at least one huge page allocated
pagesizes = set()

# Each page size may have a subset of valid wordsizes
# This is a dictionary (indexed by page size) of sets
wordsizes_by_pagesize = {}

# The linkhuge tests may only be valid on a subset of word sizes
# This set contains the wordsizes valid for linkhuge tests
linkhuge_wordsizes = set()

# A list of all discovered mountpoints that may be used by libhugetlbfs for
# this run of tests.  This is used for cleaning up left-over share files.
mounts = []

# Results matrix:  This 3-D dictionary is indexed as follows:
#   [type]     - Test results fall into one of the 'result_types' categories
#   [pagesize] - a page size from the set 'pagesizes'
#   [bits]     - a word size from the set 'wordsizes'
#   The indexed value is the number of tests matching the above traits
R = {}
result_types = ("total", "pass", "config", "fail", "xfail", "xpass",
                "signal", "strange", "skip")

def bash(cmd):
    """
    Run 'cmd' in the shell and return the exit code and output.
    """
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    try:
        rc = p.wait()
    except KeyboardInterrupt:
        # Abort and mark this a strange test result
        return (127, "")
    out = p.stdout.read().strip()
    return (rc, out)

def snapshot_pool_state():
    l = []
    for d in os.listdir("/sys/kernel/mm/hugepages"):
        substate = [(f, int(open("/sys/kernel/mm/hugepages/%s/%s" % (d, f)).read()))
                    for f in os.listdir("/sys/kernel/mm/hugepages/%s" % d)]
        l.append((d, tuple(substate)))
    return tuple(l)

def run_test_prog(bits, pagesize, cmd, **env):
    if paranoid_pool_check:
        beforepool = snapshot_pool_state()
        print "Pool state: %s" % str(beforepool)

    local_env = os.environ.copy()
    local_env.update(env)
    local_env["PATH"] = "./obj%d:../obj%d:%s" \
        % (bits, bits, local_env.get("PATH", ""))
    local_env["LD_LIBRARY_PATH"] = "../obj%d:obj%d:%s" \
        % (bits, bits, local_env.get("LD_LIBRARY_PATH", ""))
    local_env["HUGETLB_DEFAULT_PAGE_SIZE"] = repr(pagesize)

    try:
        p = subprocess.Popen(cmd, env=local_env, stdout=subprocess.PIPE)
        rc = p.wait()
    except KeyboardInterrupt:
        # Abort and mark this a strange test result
        return (None, "")
    except OSError:
        return (None, "")
    out = p.stdout.read().strip()

    if paranoid_pool_check:
        afterpool = snapshot_pool_state()
        if afterpool != beforepool:
            print >>sys.stderr, "Hugepage pool state not preserved!"
            print >>sys.stderr, "BEFORE: %s" % str(beforepool)
            print >>sys.stderr, "AFTER: %s" % str(afterpool)
            sys.exit(98)

    return (rc, out)

def setup_env(override, defaults):
    """
    Set up the environment for running commands in the shell.
    """
    # All items in override are unconditionally set or unset
    for (var, val) in override.items():
        if val == None:
            if var in os.environ:
                del os.environ[var]
        else:
            os.environ[var] = val
    # If not already set, these variables are given default values
    for (var, val) in defaults.items():
        if var not in os.environ or os.environ[var] == "":
            os.environ[var] = val

def init_results():
    """
    Define the structure of the results matrix and initialize all results to 0.
    """
    global R

    for t in result_types:
        R[t] = {}
        for p in pagesizes:
            R[t][p] = {}
            for bits in (32, 64):
                R[t][p][bits] = 0

def pretty_page_size(size):
    """
    Convert a page size to a formatted string

    Given a page size in bytes, return a string that expresses the size in
    a sensible unit (K, M, or G).
    """
    factor = 0
    while size > 1024:
        factor += 1
        size /= 1024

    if   factor == 0: return "%iB" % size
    elif factor == 1: return "%iK" % size
    elif factor == 2: return "%iM" % size
    elif factor == 3: return "%iG" % size

def print_per_size(title, values):
    """
    Print one line of test results

    Print the results of a given result type on one line.  The results for all
    page sizes and word sizes are written in a table format.
    """
    print "*%20s: " % title,
    for sz in pagesizes:
        print "%4s   %4s   " % (values[sz][32], values[sz][64]),
    print

def results_summary():
    """
    Display a summary of the test results
    """
    print "********** TEST SUMMARY"
    print "*%21s" % "",
    for p in pagesizes: print "%-13s " % pretty_page_size(p),
    print
    print "*%21s" % "",
    for p in pagesizes: print "32-bit 64-bit ",
    print

    print_per_size("Total testcases", R["total"])
    print_per_size("Skipped", R["skip"])
    print_per_size("PASS", R["pass"])
    print_per_size("FAIL", R["fail"])
    print_per_size("Killed by signal", R["signal"])
    print_per_size("Bad configuration", R["config"])
    print_per_size("Expected FAIL", R["xfail"])
    print_per_size("Unexpected PASS", R["xpass"])
    print_per_size("Strange test result", R["strange"])
    print "**********"

def free_hpages():
    """
    Return the number of free huge pages.

    Parse /proc/meminfo to obtain the number of free huge pages for
    the default page size.
    XXX: This function is not multi-size aware yet.
    """
    (rc, out) = bash("grep 'HugePages_Free:' /proc/meminfo | cut -f2 -d:")
    return (rc, int(out))

def total_hpages():
    """
    Return the total number of huge pages in the pool.

    Parse /proc/meminfo to obtain the number of huge pages for the default
    page size.
    XXX: This function is not multi-size aware yet.
    """
    (rc, out) = bash("grep 'HugePages_Total:' /proc/meminfo | cut -f2 -d:")
    return (rc, int(out))

def hpage_size():
    """
    Return the size of the default huge page size in bytes.

    Parse /proc/meminfo to obtain the default huge page size.  This number is
    reported in Kb so multiply it by 1024 to convert it to bytes.
    XXX: This function is not multi-size aware yet.
    """
    (rc, out) = bash("grep 'Hugepagesize:' /proc/meminfo | awk '{print $2}'")
    if out == "": out = 0
    out = int(out) * 1024
    return (rc, out)

def clear_hpages():
    """
    Remove stale hugetlbfs files after sharing tests.

    Traverse the mount points that are in use during testing to find left-over
    files that were created by the elflink sharing tests.  These are not
    cleaned up automatically and must be removed to free up the huge pages.
    """
    for mount in mounts:
        dir = mount + "/elflink-uid-" + `os.getuid()`
        for root, dirs, files in os.walk(dir, topdown=False):
            for name in files:
                os.remove(os.path.join(root, name))
            for name in dirs:
                os.rmdir(os.path.join(root, name))
        try:
            os.rmdir(dir)
        except OSError:
            pass

def get_pagesizes():
    """
    Get a list of configured huge page sizes.

    Use libhugetlbfs' hugeadm utility to get a list of page sizes that have
    active mount points and at least one huge page allocated to the pool.
    """
    sizes = set()
    out = ""
    (rc, out) = bash("../obj/hugeadm --page-sizes")
    if rc != 0 or out == "": return sizes

    for size in out.split("\n"): sizes.add(int(size))
    return sizes

def get_wordsizes():
    """
    Checks for obj32 and obj64 subdirs to determine valid word sizes.
    """
    sizes = set()
    if os.path.isdir("./obj32"): sizes.add(32)
    if os.path.isdir("./obj64"): sizes.add(64)

    return sizes

def check_hugetlbfs_path():
    """
    Check each combination of page size and word size for validity.

    Some word sizes may not be valid for all page sizes.  For example, a 16G
    page is too large to be used in a 32 bit process.  Use a helper program to
    weed out invalid combinations and print informational messages as required.
    """
    global wordsizes, pagesizes, mounts, wordsizes_by_pagesize

    for p in pagesizes:
        okbits = []
        for b in wordsizes:
            (rc, out) = run_test_prog(b, p, "get_hugetlbfs_path")
            if rc == 0:
                okbits.append(b)
                mounts.append(out)
        if len(okbits) == 0:
            print "run_tests.py: No mountpoints available for page size %s" % \
                  pretty_page_size(p)
            wordsizes_by_pagesize[p] = set()
            continue
        for b in wordsizes - set(okbits):
            print "run_tests.py: The %i bit word size is not compatible with " \
                  "%s pages" % (b, pretty_page_size(p))
        wordsizes_by_pagesize[p] = set(okbits)

def check_linkhuge_tests():
    """
    Check if the linkhuge tests are safe to run on this system.

    Newer versions of binutils (>= 2.18) are known to be incompatible with the
    linkhuge tests and running them may cause unreliable behavior.  Determine
    which word sizes can be tested with linkhuge.  The others will be skipped.
    NOTE: The linhuge_rw tests are always safe to run and will not be skipped.
    """
    okbits = set()

    for bits in wordsizes:
        script = open('obj%d/dummy.ldscript' % bits, 'r').read()
        if script.count('SPECIAL') == 0:
            okbits.add(bits)
    return okbits

def print_cmd(pagesize, bits, cmd, env):
    if env:
        print ' '.join(['%s=%s' % (k, v) for k, v in env.items()]),
    if type(cmd) != types.StringType:
        cmd = ' '.join(cmd)
    print "%s (%s: %i):\t" % (cmd, pretty_page_size(pagesize), bits),
    sys.stdout.flush()

def run_test(pagesize, bits, cmd, **env):
    """
    Execute a test, print the output and log the result

    Run a test using the specified page size and word size.  The parameter
    'pre' may contain additional environment settings and will be prepended to
    cmd.  A line showing info about the test is printed and after completion
    the test output is printed.  The result is recorded in the result matrix.
    """
    global R

    objdir = "obj%i" % bits
    if not os.path.isdir(objdir):
        return

    print_cmd(pagesize, bits, cmd, env)
    (rc, out) = run_test_prog(bits, pagesize, cmd, **env)
    print out

    R["total"][pagesize][bits] += 1
    if rc == 0:    R["pass"][pagesize][bits] += 1
    elif rc == 1:  R["config"][pagesize][bits] += 1
    elif rc == 2:  R["fail"][pagesize][bits] += 1
    elif rc == 3:  R["xfail"][pagesize][bits] += 1
    elif rc == 4:  R["xpass"][pagesize][bits] += 1
    elif rc < 0: R["signal"][pagesize][bits] += 1
    else:          R["strange"][pagesize][bits] += 1

def skip_test(pagesize, bits, cmd, **env):
    """
    Skip a test, print test information, and log that it was skipped.
    """
    global tot_tests, tot_skip
    R["total"][pagesize][bits] += 1
    R["skip"][pagesize][bits] += 1
    print_cmd(pagesize, bits, cmd, env)
    print "SKIPPED"

def do_test(cmd, bits=None, **env):
    """
    Run a test case, testing each page size and each indicated word size.
    """
    if bits == None: bits = wordsizes
    for p in pagesizes:
        for b in (set(bits) & wordsizes_by_pagesize[p]):
            run_test(p, b, cmd, **env)

def do_test_with_rlimit(rtype, limit, cmd, bits=None, **env):
    """
    Run a test case with a temporarily altered resource limit.
    """
    oldlimit = resource.getrlimit(rtype)
    resource.setrlimit(rtype, (limit, limit))
    do_test(cmd, bits, **env)
    resource.setrlimit(rtype, oldlimit)

def do_elflink_test(cmd, **env):
    """
    Run an elflink test case, skipping known-bad configurations.
    """
    for p in pagesizes:
        for b in wordsizes_by_pagesize[p]:
            if b in linkhuge_wordsizes: run_test(p, b, cmd, **env)
            else: skip_test(p, b, cmd, **env)

def elflink_test(cmd, **env):
    """
    Run an elflink test case with different configuration combinations.

    Test various combinations of: preloading libhugetlbfs, B vs. BDT link
    modes, minimal copying on or off, and disabling segment remapping.
    """
    do_test(cmd, **env)
    # Test we don't blow up if not linked for hugepage
    do_test(cmd, LD_PRELOAD="libhugetlbfs.so", **env)

    # Only run custom ldscript tests when -l option is set
    if not custom_ldscripts:
        return

    do_elflink_test("xB." + cmd, **env)
    do_elflink_test("xBDT." + cmd, **env)
    # Test we don't blow up if HUGETLB_MINIMAL_COPY is diabled
    do_elflink_test("xB." + cmd, HUGETLB_MINIMAL_COPY="no", **env)
    do_elflink_test("xBDT." + cmd, HUGETLB_MINIMAL_COPY="no", **env)
    # Test that HUGETLB_ELFMAP=no inhibits remapping as intended
    do_elflink_test("xB." + cmd, HUGETLB_ELFMAP="no", **env)
    do_elflink_test("xBDT." + cmd, HUGETLB_ELFMAP="no", **env)

def elflink_rw_test(cmd, **env):
    """
    Run the elflink_rw test with different configuration combinations.

    Test various combinations of: remapping modes and minimal copy on or off.
    """
    # Basic tests: None, Read-only, Write-only, Read-Write, exlicit disable
    do_test(cmd, **env)
    do_test(cmd, HUGETLB_ELFMAP="R", **env)
    do_test(cmd, HUGETLB_ELFMAP="W", **env)
    do_test(cmd, HUGETLB_ELFMAP="RW", **env)
    do_test(cmd, HUGETLB_ELFMAP="no", **env)

    # Test we don't blow up if HUGETLB_MINIMAL_COPY is disabled
    do_test(cmd, HUGETLB_MINIMAL_COPY="no", HUGETLB_ELFMAP=R"", **env)
    do_test(cmd, HUGETLB_MINIMAL_COPY="no", HUGETLB_ELFMAP="W", **env)
    do_test(cmd, HUGETLB_MINIMAL_COPY="no", HUGETLB_ELFMAP="RW", **env)

def elfshare_test(cmd, **env):
    """
    Test segment sharing with multiple configuration variations.
    """
    # Run each elfshare test invocation independently - clean up the
    # sharefiles before and after in the first set of runs, but leave
    # them there in the second:
    clear_hpages()
    do_elflink_test("xB." + cmd, HUGETLB_SHARE="1", **env)
    clear_hpages()
    do_elflink_test("xBDT." + cmd, HUGETLB_SHARE="1", **env)
    clear_hpages()
    do_elflink_test("xB." + cmd, HUGETLB_SHARE="1", **env)
    do_elflink_test("xBDT." + cmd, HUGETLB_SHARE="1", **env)
    clear_hpages()

def elflink_and_share_test(cmd, **env):
    """
    Run the ordinary linkhuge tests with sharing enabled
    """
    # Run each elflink test pair independently - clean up the sharefiles
    # before and after each pair
    clear_hpages()
    for link_str in ("xB.", "xBDT."):
        for i in range(2):
            do_elflink_test(link_str + cmd, HUGETLB_SHARE=repr(i), **env)
        clear_hpages()

def elflink_rw_and_share_test(cmd, **env):
    """
    Run the ordinary linkhuge_rw tests with sharing enabled
    """
    clear_hpages()
    for mode in ("R", "W", "RW"):
        for i in range(2):
            do_test(cmd, HUGETLB_ELFMAP=mode, HUGETLB_SHARE=repr(i), **env)
        clear_hpages()

def setup_shm_sysctl(limit):
    """
    Adjust the kernel shared memory limits to accomodate a desired size.

    The original values are returned in a dictionary that can be passed to
    restore_shm_sysctl() to restore the system state.
    """
    if os.getuid() != 0: return {}
    sysctls = {}
    files = [ "/proc/sys/kernel/shmmax", "/proc/sys/kernel/shmall"]
    for f in files:
        fh = open(f, "r")
        sysctls[f] = fh.read()
        fh.close()
        fh = open(f, "w")
        fh.write(`limit`)
        fh.close()
    print "set shmmax limit to %s" % limit
    return sysctls

def restore_shm_sysctl(sysctls):
    """
    Restore the sysctls named in 'sysctls' to the given values.
    """
    if os.getuid() != 0: return
    for (file, val) in sysctls.items():
        fh = open(file, "w")
        fh.write(val)
        fh.close()

def do_shm_test(cmd, limit=None, bits=None, **env):
    """
    Run a test case with temporarily expanded SysV shm limits, testing
    each indicated word size.
    """
    if bits == None:
        bits = wordsizes
    if limit != None:
        tmp = setup_shm_sysctl(limit)
    for b in bits:
        run_test(system_default_hpage_size, b, cmd, **env)
    if limit != None:
        restore_shm_sysctl(tmp)

def functional_tests():
    """
    Run the set of functional tests.
    """
    global linkhuge_wordsizes

    # Kernel background tests not requiring hugepage support
    do_test("zero_filesize_segment")

    # Library background tests not requiring hugepage support
    do_test("test_root")
    do_test("meminfo_nohuge")

    # Library tests requiring kernel hugepage support
    do_test("gethugepagesize")
    do_test("gethugepagesizes")
    do_test("empty_mounts", HUGETLB_VERBOSE="1")
    do_test("large_mounts", HUGETLB_VERBOSE="1")

    # Tests requiring an active and usable hugepage mount
    do_test("find_path")
    do_test("unlinked_fd")
    do_test("readback")
    do_test("truncate")
    do_test("shared")
    do_test("mprotect")
    do_test_with_rlimit(resource.RLIMIT_MEMLOCK, -1, "mlock")
    do_test("misalign")
    do_test("fallocate_basic.sh")
    do_test("fallocate_align.sh")

    # Specific kernel bug tests
    do_test("ptrace-write-hugepage")
    do_test("icache-hygiene")
    do_test("slbpacaflush")
    do_test("straddle_4GB_static", bits=(64,))
    do_test("huge_at_4GB_normal_below_static", bits=(64,))
    do_test("huge_below_4GB_normal_above_static", bits=(64,))
    do_test("map_high_truncate_2")
    do_test("misaligned_offset")
    do_test("truncate_above_4GB")
    do_test("brk_near_huge")
    do_test("task-size-overrun")
    do_test_with_rlimit(resource.RLIMIT_STACK, -1, "stack_grow_into_huge")
    do_test("corrupt-by-cow-opt")
    do_test("noresv-preserve-resv-page")
    do_test("noresv-regarded-as-resv")

    if dangerous == 1:
        do_test("readahead_reserve")
        do_test("madvise_reserve")
        do_test("fadvise_reserve")
        do_test("mremap-expand-slice-collision")
        do_test("mremap-fixed-normal-near-huge")
        do_test("mremap-fixed-huge-near-normal")
    else:
        do_test("readahead_reserve.sh")
        do_test("madvise_reserve.sh")
        do_test("fadvise_reserve.sh")
        do_test("mremap-expand-slice-collision.sh")
        do_test("mremap-fixed-normal-near-huge.sh")
        do_test("mremap-fixed-huge-near-normal.sh")
    do_shm_test("shm-perms", 64*1024*1024)

    # Tests requiring an active mount and hugepage COW
    do_test("private")
    do_test("fork-cow")
    do_test("direct")
    do_test("malloc")
    do_test("malloc", LD_PRELOAD="libhugetlbfs.so", HUGETLB_MORECORE="yes")
    do_test("malloc", LD_PRELOAD="libhugetlbfs.so", HUGETLB_MORECORE="yes",
            HUGETLB_RESTRICT_EXE="unknown:none")
    do_test("malloc", LD_PRELOAD="libhugetlbfs.so", HUGETLB_MORECORE="yes",
            HUGETLB_RESTRICT_EXE="unknown:malloc")
    do_test("malloc_manysmall")
    do_test("malloc_manysmall", LD_PRELOAD="libhugetlbfs.so",
            HUGETLB_MORECORE="yes")
    do_test("heapshrink")
    do_test("heapshrink", LD_PRELOAD="libheapshrink.so")
    do_test("heapshrink", LD_PRELOAD="libhugetlbfs.so", HUGETLB_MORECORE="yes")
    do_test("heapshrink", LD_PRELOAD="libhugetlbfs.so libheapshrink.so",
            HUGETLB_MORECORE="yes")
    do_test("heapshrink", LD_PRELOAD="libheapshrink.so", HUGETLB_MORECORE="yes",
            HUGETLB_MORECORE_SHRINK="yes")
    do_test("heapshrink", LD_PRELOAD="libhugetlbfs.so libheapshrink.so",
            HUGETLB_MORECORE="yes", HUGETLB_MORECORE_SHRINK="yes")
    do_test("heap-overflow", HUGETLB_VERBOSE="1", HUGETLB_MORECORE="yes")

    # Run the remapping tests' up-front checks
    linkhuge_wordsizes = check_linkhuge_tests()
    # Original elflink tests
    elflink_test("linkhuge_nofd", HUGETLB_VERBOSE="0")
    elflink_test("linkhuge")

    # Only run custom ldscript tests when -l option is set
    if custom_ldscripts:
        # Original elflink sharing tests
        elfshare_test("linkshare")
        elflink_and_share_test("linkhuge")

    # elflink_rw tests
    elflink_rw_test("linkhuge_rw")
    # elflink_rw sharing tests
    elflink_rw_and_share_test("linkhuge_rw")

    # Accounting bug tests
    # reset free hpages because sharing will have held some
    # alternatively, use
    do_test("chunk-overcommit")
    do_test(("alloc-instantiate-race", "shared"))
    do_test(("alloc-instantiate-race", "private"))
    do_test("truncate_reserve_wraparound")
    do_test("truncate_sigbus_versus_oom")

    # Test direct allocation API
    do_test("get_huge_pages")

    # Test overriding of shmget()
    do_shm_test("shmoverride_linked")
    do_shm_test("shmoverride_linked", HUGETLB_SHM="yes")
    do_shm_test("shmoverride_linked_static")
    do_shm_test("shmoverride_linked_static", HUGETLB_SHM="yes")
    do_shm_test("shmoverride_unlinked", LD_PRELOAD="libhugetlbfs.so")
    do_shm_test("shmoverride_unlinked", LD_PRELOAD="libhugetlbfs.so", HUGETLB_SHM="yes")

    # Test hugetlbfs filesystem quota accounting
    do_test("quota.sh")

    # Test accounting of HugePages_{Total|Free|Resv|Surp}
    #  Alters the size of the hugepage pool so should probably be run last
    do_test("counters.sh")

def stress_tests():
    """
    Run the set of stress tests.
    """
    iterations = 10	# Number of iterations for looping tests

    # Don't update NRPAGES every time like above because we want to catch the
    # failures that happen when the kernel doesn't release all of the huge pages
    # after a stress test terminates
    (rc, nr_pages) = free_hpages()

    do_test(("mmap-gettest", repr(iterations), repr(nr_pages)))

    # mmap-cow needs a hugepages for each thread plus one extra
    do_test(("mmap-cow", repr(nr_pages-1), repr(nr_pages)))

    (rc, tot_pages) = total_hpages()
    limit = system_default_hpage_size * tot_pages
    threads = 10	# Number of threads for shm-fork

    # Run shm-fork once using half available hugepages, then once using all
    # This is to catch off-by-ones or races in the kernel allocated that
    # can make allocating all hugepages a problem
    if nr_pages > 1:
        do_shm_test(("shm-fork", repr(threads), repr(nr_pages / 2)), limit)
    do_shm_test(("shm-fork", repr(threads), repr(nr_pages)), limit)

    do_shm_test(("shm-getraw", repr(nr_pages), "/dev/full"), limit)

    do_test("fallocate_stress.sh")

def main():
    global wordsizes, pagesizes, dangerous, paranoid_pool_check, system_default_hpage_size
    global custom_ldscripts
    testsets = set()
    env_override = {"QUIET_TEST": "1", "HUGETLBFS_MOUNTS": "",
                    "HUGETLB_ELFMAP": None, "HUGETLB_MORECORE": None}
    env_defaults = {"HUGETLB_VERBOSE": "0"}
    dangerous = 0
    paranoid_pool_check = False
    custom_ldscripts = False

    try:
        opts, args = getopt.getopt(sys.argv[1:], "vVfdt:b:p:c:l")
    except getopt.GetoptError, err:
        print str(err)
        sys.exit(1)
    for opt, arg in opts:
       if opt == "-v":
           env_override["QUIET_TEST"] = None
           env_defaults["HUGETLB_VERBOSE"] = "2"
       elif opt == "-V":
           env_defaults["HUGETLB_VERBOSE"] = "99"
       elif opt == "-f":
           dangerous = 1
       elif opt == "-t":
           for t in arg.split(): testsets.add(t)
       elif opt == "-b":
           for b in arg.split(): wordsizes.add(int(b))
       elif opt == "-p":
           for p in arg.split(): pagesizes.add(int(p))
       elif opt == '-c':
           paranoid_pool_check = True
       elif opt == '-l':
           custom_ldscripts = True
       else:
           assert False, "unhandled option"
    if len(testsets) == 0: testsets = set(["func", "stress"])
    if len(wordsizes) == 0: wordsizes = get_wordsizes()
    if len(pagesizes) == 0: pagesizes = get_pagesizes()

    if len(pagesizes) == 0:
        print "Unable to find available page sizes, are you sure hugetlbfs"
        print "is mounted and there are available huge pages?"
        return 1

    setup_env(env_override, env_defaults)
    init_results()

    (rc, system_default_hpage_size) = hpage_size()
    if rc != 0:
        print "Unable to find system default hugepage size."
        print "Is hugepage supported included in this kernel?"
        return 1

    check_hugetlbfs_path()

    if "func" in testsets: functional_tests()
    if "stress" in testsets: stress_tests()

    results_summary()

if __name__ == "__main__":
    main()
