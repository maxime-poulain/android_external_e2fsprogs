/*
 * alloc_tables.c --- Allocate tables for a newly initialized
 * filesystem.  Used by mke2fs when initializing a filesystem
 *
 * Copyright (C) 1996 Theodore Ts'o.
 *
 * %Begin-Header%
 * This file may be redistributed under the terms of the GNU Public
 * License.
 * %End-Header%
 */

#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include <linux/ext2_fs.h>

#include "ext2fs.h"

errcode_t ext2fs_allocate_group_table(ext2_filsys fs, int group,
				      ext2fs_block_bitmap bmap)
{
	errcode_t	retval;
	blk_t		group_blk, start_blk, last_blk, new_blk, blk;
	int		j;

	group_blk = fs->super->s_first_data_block +
		(group * fs->super->s_blocks_per_group);
	
	last_blk = group_blk + fs->super->s_blocks_per_group;
	if (last_blk >= fs->super->s_blocks_count)
		last_blk = fs->super->s_blocks_count - 1;

	start_blk = group_blk + 3 + fs->desc_blocks;
	if (start_blk > last_blk)
		start_blk = group_blk;

	if (!bmap)
		bmap = fs->block_map;
	
	/*
	 * Allocate the inode table
	 */
	if (!fs->group_desc[group].bg_inode_table) {
		retval = ext2fs_get_free_blocks(fs, start_blk, last_blk,
						fs->inode_blocks_per_group,
						bmap, &new_blk);
		if (retval)
			return retval;
		for (j=0, blk = new_blk;
		     j < fs->inode_blocks_per_group;
		     j++, blk++)
			ext2fs_mark_block_bitmap(bmap, blk);
		fs->group_desc[group].bg_inode_table = new_blk;
	}

	/*
	 * Allocate the block and inode bitmaps, if necessary
	 */
	if (fs->stride) {
		start_blk += fs->inode_blocks_per_group;
		start_blk += ((fs->stride * group) %
			      (last_blk - start_blk));
		if (start_blk > last_blk)
			/* should never happen */
			start_blk = group_blk;
	} else
		start_blk = group_blk;

	if (!fs->group_desc[group].bg_block_bitmap) {
		retval = ext2fs_get_free_blocks(fs, start_blk, last_blk,
						1, bmap, &new_blk);
		if (retval)
			return retval;
		ext2fs_mark_block_bitmap(bmap, new_blk);
		fs->group_desc[group].bg_block_bitmap = new_blk;
	}

	if (!fs->group_desc[group].bg_inode_bitmap) {
		retval = ext2fs_get_free_blocks(fs, start_blk, last_blk,
						1, bmap, &new_blk);
		if (retval)
			return retval;
		ext2fs_mark_block_bitmap(bmap, new_blk);
		fs->group_desc[group].bg_inode_bitmap = new_blk;
	}
	return 0;
}

	

errcode_t ext2fs_allocate_tables(ext2_filsys fs)
{
	errcode_t	retval;
	int		i;

	for (i = 0; i < fs->group_desc_count; i++) {
		retval = ext2fs_allocate_group_table(fs, i, fs->block_map);
		if (retval)
			return retval;
	}
	return 0;
}

