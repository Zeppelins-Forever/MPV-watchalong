# MPV-watchalong

### Watch two videos at once, while controlling both!

<p align="center">
<img src="https://github.com/Zeppelins-Forever/MPV-watchalong/blob/icon&comments/icons/app-icon.png?raw=true" alt="Description" width="150" height="150" align="center">
</p>

You can select any two videos and play them in two seperate MPV-based windows, while controlling the most common settings on each (or on both at the same time)! Works on Windows, MacOS, and Linux (with the KDE Plasma desktop being the most well-supported due to buing built in Qt, but it should run mostly fine in other desktops too).

### Note: when you're closing videos, ALWAYS press the in-app "Close" button. The MPV API gets a little funky if you don't, especially on MacOS!

Perfect for comparing the output of a video processing software!

Or, if you've downloaded the VOD and the media, perfect for watching watchalong streams!

![Mori Calliope "The Hogfather" Watchalong](https://github.com/Zeppelins-Forever/MPV-watchalong/blob/main/images/Calli-Hogfather-example.png?raw=true)

## Quirks:

- As stated above, you should ALWAYS close videos via the "Close" button, not by closing the video window itself. It's weird, I know.
- If you use this on MacOS (or any other system which may explicitly ask for permission for an app to access a directory), this app may crash on first access. MacOS prevents the app from accessing the desired directory until you press "Allow", but this currently will ALWAYS cause the app to hang. Force the app to shutdown by right-clicking the app in the dock, and forcing it to close. It should now have access to that directory, and should run perfectly fine!

![Close-Up of MPV-watchalong Interface](https://github.com/Zeppelins-Forever/MPV-watchalong/blob/main/images/mpv-watchalong-menu.png?raw=true)

---

### To-do

- Add icons on the buttons, i.e. Pause/Play
- Try to add dark mode explicit toggle for non-Qt-based systems.
- Try to make statically linked, single-executable Windows EXE.
- Maybe move the "close" button somewhere more out-of-the-way.
- Fix hangup during MacOS file selection when directory access permission is requested.
