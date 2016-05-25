#!/bin/sh

destdir=/recordings

my_umount()
{
        umount -f "${destdir}"
}

my_mount()
{
        if ! mount -t auto -o noexec,rw,async "/dev/$1" "${destdir}"; then
                # failed to mount
                exit 1
        fi
}

case "${ACTION}" in
add)
        my_umount ${MDEV}
        my_mount ${MDEV}
        ;;
remove)
        my_umount ${MDEV}
        ;;
*)
	;;
esac
