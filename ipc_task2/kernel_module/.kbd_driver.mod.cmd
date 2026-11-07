savedcmd_kbd_driver.mod := printf '%s\n'   kbd_driver.o | awk '!x[$$0]++ { print("./"$$0) }' > kbd_driver.mod
