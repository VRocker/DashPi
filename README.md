# DashPi
Making a DashCam using a Pi Zero

# Cloning the repository
DashPi makes use of Buildroot which is added a submodule so once you have cloned this repo you will need to initialise the submodules. This is done by runnign the following commands:

```
  git submodule init
  git submodule update
```

# Building
The first time you build DashPi you will need to initialise Buildroot with our configuration files, this is done by running ```make setup```
This wil lcopy across our makefiles and build Buildroot for the first time.

Once the first build has run, simply run ```make```. This will build the custom packages followed by buildroot. Everything will then be tied together an a zImage will be created in the root directory
