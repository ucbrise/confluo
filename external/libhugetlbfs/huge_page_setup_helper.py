#!/usr/bin/python

#
# Tool to set up Linux large page support with minimal effort
#
# by Jarod Wilson <jarod@redhat.com>
# (c) Red Hat, Inc., 2009
#
# Requires hugeadm from libhugetlbfs 2.7 (or backported support)
#
import os

debug = False

# must be executed under the root to operate
if os.geteuid() != 0:
    print "You must be root to setup hugepages!"
    os._exit(1)

# config files we need access to
sysctlConf = "/etc/sysctl.conf"
if not os.access(sysctlConf, os.W_OK):
    print "Cannot access %s" % sysctlConf
    if debug == False:
        os._exit(1)

# This file will be created if it doesn't exist
limitsConf = "/etc/security/limits.d/hugepages.conf"


# Figure out what we've got in the way of memory
memTotal = 0
hugePageSize = 0
hugePages = 0

hugeadmexplain = os.popen("/usr/bin/hugeadm --explain 2>/dev/null").readlines()

for line in hugeadmexplain:
    if line.startswith("Total System Memory:"):
        memTotal = int(line.split()[3])
        break

if memTotal == 0:
    print "Your version of libhugetlbfs' hugeadm utility is too old!"
    os._exit(1)


# Pick the default huge page size and see how many pages are allocated
poolList = os.popen("/usr/bin/hugeadm --pool-list").readlines()
for line in poolList:
    if '*' in line:
        hugePageSize = int(line.split()[0])
        hugePages = int(line.split()[2])
        break

if hugePageSize == 0:
    print "Aborting, cannot determine system huge page size!"
    os._exit(1)

# Get initial sysctl settings
shmmax = 0
hugeGID = 0

for line in hugeadmexplain:
    if line.startswith("A /proc/sys/kernel/shmmax value of"):
        shmmax = int(line.split()[4])
        break

for line in hugeadmexplain:
    if line.strip().startswith("vm.hugetlb_shm_group = "):
        hugeGID = int(line.split()[2])
        break


# translate group into textual version
hugeGIDName = "null"
groupNames = os.popen("/usr/bin/getent group").readlines()
for line in groupNames:
    curGID = int(line.split(":")[2])
    if curGID == hugeGID:
        hugeGIDName = line.split(":")[0]
        break


# dump system config as we see it before we start tweaking it
print "Current configuration:"
print " * Total System Memory......: %6d MB" % memTotal
print " * Shared Mem Max Mapping...: %6d MB" % (shmmax / (1024 * 1024))
print " * System Huge Page Size....: %6d MB" % (hugePageSize / (1024 * 1024))
print " * Number of Huge Pages.....: %6d"    % hugePages
print " * Total size of Huge Pages.: %6d MB" % (hugePages * hugePageSize / (1024 * 1024))
print " * Remaining System Memory..: %6d MB" % (memTotal - (hugePages * hugePageSize / (1024 * 1024)))
print " * Huge Page User Group.....:  %s (%d)" % (hugeGIDName, hugeGID)
print


# ask how memory they want to allocate for huge pages
userIn = None
while not userIn:
    try:
        userIn = raw_input("How much memory would you like to allocate for huge pages? "
                           "(input in MB, unless postfixed with GB): ")
        if userIn[-2:] == "GB":
            userHugePageReqMB = int(userIn[0:-2]) * 1024
        elif userIn[-1:] == "G":
            userHugePageReqMB = int(userIn[0:-1]) * 1024
        elif userIn[-2:] == "MB":
            userHugePageReqMB = int(userIn[0:-2])
        elif userIn[-1:] == "M":
            userHugePageReqMB = int(userIn[0:-1])
        else:
            userHugePageReqMB = int(userIn)
        # As a sanity safeguard, require at least 128M not be allocated to huge pages
        if userHugePageReqMB > (memTotal - 128):
            userIn = None
            print "Refusing to allocate %d, you must leave at least 128MB for the system" % userHugePageReqMB
        elif userHugePageReqMB < (hugePageSize / (1024 * 1024)):
            userIn = None
            print "Sorry, allocation must be at least a page's worth!"
        else:
            break
    except ValueError:
        userIn = None
        print "Input must be an integer, please try again!"
userHugePageReqKB = userHugePageReqMB * 1024
userHugePagesReq = userHugePageReqKB / (hugePageSize / 1024)
print "Okay, we'll try to allocate %d MB for huge pages..." % userHugePageReqMB
print


# some basic user input validation
badchars = list(' \\\'":;~`!$^&*(){}[]?/><,')
inputIsValid = False
# ask for the name of the group allowed access to huge pages
while inputIsValid == False:
    foundbad = False
    userGroupReq = raw_input("What group should have access to the huge pages?"
                             "(The group will be created, if need be) [hugepages]: ")
    if userGroupReq is '':
        userGroupReq = 'hugepages'
    if userGroupReq[0].isdigit() or userGroupReq[0] == "-":
        foundbad = True
        print "Group names cannot start with a number or dash, please try again!"
    for char in badchars:
        if char in userGroupReq:
            foundbad = True
            print "Illegal characters in group name, please try again!"
            break
    if len(userGroupReq) > 16:
        foundbad = True
        print "Group names can't be more than 16 characaters, please try again!"
    if foundbad == False:
        inputIsValid = True
print "Okay, we'll give group %s access to the huge pages" % userGroupReq


# see if group already exists, use it if it does, if not, create it
userGIDReq = -1
for line in groupNames:
    curGroupName = line.split(":")[0]
    if curGroupName == userGroupReq:
        userGIDReq = int(line.split(":")[2])
        break

if userGIDReq > -1:
    print "Group %s (gid %d) already exists, we'll use it" % (userGroupReq, userGIDReq)
else:
    if debug == False:
        os.popen("/usr/sbin/groupadd %s" % userGroupReq)
    else:
        print "/usr/sbin/groupadd %s" % userGroupReq
    groupNames = os.popen("/usr/bin/getent group %s" % userGroupReq).readlines()
    for line in groupNames:
        curGroupName = line.split(":")[0]
        if curGroupName == userGroupReq:
            userGIDReq = int(line.split(":")[2])
            break
    print "Created group %s (gid %d) for huge page use" % (userGroupReq, userGIDReq)
print


# basic user input validation, take 2
# space is valid in this case, wasn't in the prior incarnation
badchars = list('\\\'":;~`!$^&*(){}[]?/><,')
inputIsValid = False
# ask for user(s) that should be in the huge page access group
while inputIsValid == False:
    foundbad = False
    userUsersReq = raw_input("What user(s) should have access to the huge pages (space-delimited list, users created as needed)? ")
    for char in badchars:
        if char in userUsersReq:
            foundbad = True
            print "Illegal characters in user name(s) or invalid list format, please try again!"
            break
    for n in userUsersReq.split():
        if len(n) > 32:
            foundbad = True
            print "User names can't be more than 32 characaters, please try again!"
            break
        if n[0] == "-":
            foundbad = True
            print "User names cannot start with a dash, please try again!"
            break
    if foundbad == False:
        inputIsValid = True
# see if user(s) already exist(s)
curUserList = os.popen("/usr/bin/getent passwd").readlines()
hugePageUserList = userUsersReq.split()
for hugeUser in hugePageUserList:
    userExists = False
    for line in curUserList:
        curUser = line.split(":")[0]
        if curUser == hugeUser:
            print "Adding user %s to huge page group" % hugeUser
            userExists = True
            if debug == False:
                os.popen("/usr/sbin/usermod -a -G %s %s" % (userGroupReq, hugeUser))
            else:
                print "/usr/sbin/usermod -a -G %s %s" % (userGroupReq, hugeUser)
        if userExists == True:
            break
    if userExists == False:
        print "Creating user %s with membership in huge page group" % hugeUser
        if debug == False:
            if hugeUser == userGroupReq:
                os.popen("/usr/sbin/useradd %s -g %s" % (hugeUser, userGroupReq))
            else:
                os.popen("/usr/sbin/useradd %s -G %s" % (hugeUser, userGroupReq))
        else:
            print "/usr/sbin/useradd %s -G %s" % (hugeUser, userGroupReq)
print


# set values for the current running environment
if debug == False:
    os.popen("/usr/bin/hugeadm --pool-pages-min DEFAULT:%sM" % userHugePageReqMB)
    os.popen("/usr/bin/hugeadm --pool-pages-max DEFAULT:%sM" % userHugePageReqMB)
    os.popen("/usr/bin/hugeadm --set-shm-group %d" % userGIDReq)
    os.popen("/usr/bin/hugeadm --set-recommended-shmmax")
else:
    print "/usr/bin/hugeadm --pool-pages-min DEFAULT:%sM" % userHugePageReqMB
    print "/usr/bin/hugeadm --pool-pages-max DEFAULT:%sM" % userHugePageReqMB
    print "/usr/bin/hugeadm --set-shm-group %d" % userGIDReq
    print "/usr/bin/hugeadm --set-recommended-shmmax"
    print

# figure out what that shmmax value we just set was
hugeadmexplain = os.popen("/usr/bin/hugeadm --explain 2>/dev/null").readlines()
for line in hugeadmexplain:
    if line.strip().startswith("kernel.shmmax = "):
        shmmax = int(line.split()[2])
        break

# write out sysctl config changes to persist across reboot
if debug == False:
    sysctlConfLines = "# sysctl configuration\n"
    if os.access(sysctlConf, os.W_OK):
        try:
            sysctlConfLines = open(sysctlConf).readlines()
            os.rename(sysctlConf, sysctlConf + ".backup")
            print("Saved original %s as %s.backup" % (sysctlConf, sysctlConf))
        except:
            pass

    fd = open(sysctlConf, "w")
    for line in sysctlConfLines:
        if line.startswith("kernel.shmmax"):
            continue
        elif line.startswith("vm.nr_hugepages"):
            continue
        elif line.startswith("vm.hugetlb_shm_group"):
            continue
        else:
            fd.write(line);

    fd.write("kernel.shmmax = %d\n" % shmmax)
    fd.write("vm.nr_hugepages = %d\n" % userHugePagesReq)
    fd.write("vm.hugetlb_shm_group = %d\n" % userGIDReq)
    fd.close()

else:
    print "Add to %s:" % sysctlConf
    print "kernel.shmmax = %d" % shmmax
    print "vm.nr_hugepages = %d" % userHugePagesReq
    print "vm.hugetlb_shm_group = %d" % userGIDReq
    print


# write out limits.conf changes to persist across reboot
if debug == False:
    limitsConfLines = "# Huge page access configuration\n"
    if os.access(limitsConf, os.W_OK):
        try:
            limitsConfLines = open(limitsConf).readlines()
            os.rename(limitsConf, limitsConf + ".backup")
            print("Saved original %s as %s.backup" % (limitsConf, limitsConf))
        except:
            pass

    fd = open(limitsConf, "w")
    for line in limitsConfLines:
        cfgExist = False
        for hugeUser in hugePageUserList:
            try:
                if line.split()[0] == hugeUser:
                    cfgExist = True
            except IndexError:
                # hit either white or comment line, it is safe not to take
                # any action and continue.
                pass
        if cfgExist == True:
            continue
        else:
            fd.write(line)

    for hugeUser in hugePageUserList:
        fd.write("%s		soft	memlock		%d\n" % (hugeUser, userHugePageReqKB))
        fd.write("%s		hard	memlock		%d\n" % (hugeUser, userHugePageReqKB))
    fd.close()

else:
    print "Add to %s:" % limitsConf
    for hugeUser in hugePageUserList:
        print "%s		soft	memlock		%d" % (hugeUser, userHugePageReqKB)
        print "%s		hard	memlock		%d" % (hugeUser, userHugePageReqKB)


# dump the final configuration of things now that we're done tweaking
print
print "Final configuration:"
print " * Total System Memory......: %6d MB" % memTotal
if debug == False:
    print " * Shared Mem Max Mapping...: %6d MB" % (shmmax / (1024 * 1024))
else:
    # This should be what we *would* have set it to, had we actually run hugeadm --set-recommended-shmmax
    print " * Shared Mem Max Mapping...: %6d MB" % (userHugePagesReq * hugePageSize / (1024 * 1024))
print " * System Huge Page Size....: %6d MB" % (hugePageSize / (1024 * 1024))
print " * Available Huge Pages.....: %6d"    % userHugePagesReq
print " * Total size of Huge Pages.: %6d MB" % (userHugePagesReq * hugePageSize / (1024 * 1024))
print " * Remaining System Memory..: %6d MB" % (memTotal - userHugePageReqMB)
print " * Huge Page User Group.....:  %s (%d)" % (userGroupReq, userGIDReq)
print

