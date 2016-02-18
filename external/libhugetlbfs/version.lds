VERS_1.0 {
	global:
		gethugepagesize;
		hugetlbfs_test_path;
		hugetlbfs_find_path;
		hugetlbfs_unlinked_fd;
	local:
		direct_syscall;
		__lh_*;
		__pu_*;
};

HTLBFS_2.0 {
	global:
		get_huge_pages;
		free_huge_pages;
};

HTLBFS_2.1 {
	global:
		get_hugepage_region;
		free_hugepage_region;
		gethugepagesizes;
		getpagesizes;
		hugetlbfs_find_path_for_size;
		hugetlbfs_unlinked_fd_for_size;
		__tp_*;
};
