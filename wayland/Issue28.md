# Trying to Help! (But, I'm a bad programmer.)

This write-up can also be found in the "wayland" branch, under the "wayland"
directory as "Issue28.md"

## Thank You

To start, I am a user of awakened-poe-trade and I would like to thank you for
making such a great tool!

## Background

I recently switched from [Linux Mint](https://www.linuxmint.com/) (X11) to
[Hyprland](https://hyprland.org/) (Wayland).
APT worked flawless in Mint/X11, but I couldn't get it to work well in Wayland.

I looked through the issues on APT to see if I could find anything and then
realized APT seems to rely on electron-overlay-window.

I then found this Issue #28 when troubleshooting and decided I would *try* to
help as best I could.

I tried to see if I could figure things out, but realized I was in over my head,
but maybe I could compile the information for you to figure things out.

### Willing to Help

**Please let me know how I can help!**

I can run tests for you, hunt down information, etc.
My skills as a programmer, mostly not knowing JS and being weak in C, hinder my
ability to just take this and fully find a solution. But, I can take orders,
follow instructions, and I'm willing to try to help! It would be great if we
can get APT working in Wayland before the league start.

My pc can be booted into Windows 10, Linux Mint (X11), Arch Linux with Hyprland
(Wayland), and Arch Linux with KDE Plasma (Wayland). I can run various tests as
needed but will need detailed instructions to do so. I managed to get the demo
to "load" but it fails to hook in to things with Wayland.

## References

Links to references that may help:

- [Wayland Explorer-Wayland](https://wayland.app/protocols/wayland)
    Breakdown of the Wayland protocol in an easier to read format than XML.

- [Wayland Explorer-XDG shell](https://wayland.app/protocols/xdg-shell)
    Breakdown of the XDG shell protocol. Cross-Desktop-Group (XDG) helps make
    things usable across different desktop environments so you don't need to
    program things for specific environments.

- [Wayland Protocol Book](https://wayland-book.com/introduction.html)
    Book describing Wayland protocols. Not finished as it stops at ~10.4.

- [hello-wayland - Example Code](https://gitlab.freedesktop.org/emersion/hello-wayland)
    This repo shows how to make a wayland window and generate the protocol files
    with a Makefile. I have included a modified version of main.c, and a
    generated protocol files for reference.

- [zero to window - YouTube Video](https://www.youtube.com/watch?v=iIVIu7YRdY0)
    The person in this video gives a full example of starting with nothing and
    ending with a working wayland window.

## Overlay X11 Requirements

From what I have seen stated in README.md, you need:

- [_NET_ACTIVE_WINDOW](https://specifications.freedesktop.org/wm-spec/latest/ar01s03.html#idm45534522230800)
- [_NET_WM_STATE_FULLSCREEN](https://specifications.freedesktop.org/wm-spec/latest/ar01s05.html#idm45534522147488)
- [_NET_WM_NAME](https://specifications.freedesktop.org/wm-spec/latest/ar01s05.html#idm45534522174688)

for the X11 implementation?

Let me know if that's correct, and if there's other things you need? I will
attempt to find/translate them for you.

Something you may need to look into is how Wayland does global shortcuts.
See [Global Shortcuts](#global-shortcuts) for more information.

## X11 Reqs -> Wayland

What I am attempting to do here is translate the X11 requirements to their
Wayland versions and show how *I understand them* to work.

My intention is that you will see these and understand how to implement them
into the overlay app.

### _NET_ACTIVE_WINDOW

I need more details for this one. Are you using this to see if the overlay is
active? Or do you use it to see if the window you are trying to attach to is
active?

### _NET_WM_STATE_FULLSCREEN

[xdg_toplevel:set_fullscreen](https://wayland.app/protocols/xdg-shell#xdg_toplevel:request:set_fullscreen)

Example of this can be seen in main.c line 106:

`xdg_toplevel_set_fullscreen(xdg_toplevel, NULL);`

Line 112 shows unsetting:

`xdg_toplevel_unset_fullscreen(xdg_toplevel);`

### _NET_WM_NAME

[xdg_toplevel:set_title](https://wayland.app/protocols/xdg-shell#xdg_toplevel:request:set_fullscreen)

Example of this can be seen in main.c line 244:

`xdg_toplevel_set_title(xdg_toplevel, "Hello Wayland Ac5000");`

## Global Shortcuts

For security purposes, Wayland doesn't just give **every** window the current
keystrokes. (Keyloggers for example.) The application has to have focus.

Because of this, [xdg-desktop-portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.impl.portal.GlobalShortcuts.html#description)
can register shortcuts for your application.

Electron is aware of this as well and has [this open issue](https://github.com/electron/electron/issues/38288)
on their tracker. I suspect this may get fixed by them soon considering most
Linux distros are now ready to move fully into Wayland with the Nvidia explicit
sync fix recently.

## Included Files

In the wayland directory of the wayland branch, I have included a few files.

- main.c
- xdg-shell-client-protocol.h
- xdg-shell-protocol.c
- Makefile

`main.c` is a modified version of the same file from [here](https://gitlab.freedesktop.org/emersion/hello-wayland).
You will need to look at the repo to see how to run the program fully, but I
believe it's a good starting file for showing the basics. The YouTube video
link in the [References](#references) section ended up with a very similar
looking file.

`xdg-shell-client-protocol.h` and `xdg-shell-protocol.c` are generated via the
"wayland-scanner" binary that comes with the wayland package on Linux. The
`Makefile` shows how this is done on line 9.
