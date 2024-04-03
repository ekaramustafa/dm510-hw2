savedcmd_/home/ebu/dm510/assignment2/dm510_dev.mod := printf '%s\n'   dm510_dev.o | awk '!x[$$0]++ { print("/home/ebu/dm510/assignment2/"$$0) }' > /home/ebu/dm510/assignment2/dm510_dev.mod
