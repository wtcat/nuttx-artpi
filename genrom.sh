#!/bin/sh

genromfs -f romfs_img -d etc -V nuttx
xxd -i romfs_img > apps/nshlib/nsh_romfsimg.h
rm -f romfs_img
