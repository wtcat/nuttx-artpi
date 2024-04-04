#!/bin/sh

genromfs -f romfs_img -d etc -V nuttx
xxd -i romfs_img > nuttx/nsh_romfsimg.h
rm -f romfs_img
