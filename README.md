# netlink nl80211 wireless client delete example aka NL80211_CMD_DEL_STATION

Almost no dependencies utility, except `libnl-3`.

## Howto build
libnl-3-dev libnl-genl-3-dev packets should be installed.
```
git clone <repo_url>
cd <repo_dir>
make station_del
```
Binary is in the `build` directory

## Howto use
```
┌──(ru㉿kali)-[~/netlink_station_del]
└─$ make
gcc -Wall -O2 -I./ -I/usr/include/libnl3  main.c -o build/station_del -lnl-3 -lnl-genl-3

┌──(ru㉿kali)-[~/netlink_station_del]
└─$ sudo build/station_del dev wlan0 mac 50:3d:c6:54:77:c1
nl80211 driver id: 31

┌──(ru㉿kali)-[~/netlink_station_del]
└─$

```
