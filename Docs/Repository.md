# Debian Repository

Thanks to the people over at [GemFury](https://gemfury.com/) who offer
free public repositories of all kinds I can offer an easy way to 
install HamChrono and keep updated.

On your Raspberry Pi or x86 Debian based distribution create this file:
`/etc/apt/sources.list.d/ve3ysh.list`

And insert this line:
`deb [trusted=yes] https://apt.fury.io/ve3ysh/ /`

Then perform the following:
```
sudo apt update
sudo apt install hamchrono
```