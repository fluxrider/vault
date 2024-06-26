# Vault
Keeps your passwords and and notes encrypted (using the modern at time of writing (2022), ARGON2 key derivation and AEAD/xchacha20poly1305 encryption).

## Rational
I had been using KeePassXC for two years, and suddently it corrupted my password file. Even when reverting to an old backup of the file, modifying anything corrupts it. It's not like I can send the keepassxc team my password file for them to test. Also, like pretty much all project out there, their code is too messy, too many files, too many lines, too many features. With my fibrofog it's a lost cause.

I tried a few online password services, but found that LastPass 'forgot' some new entries if the connection to the internet was spotty, and RoboForm kept guessing wrong the url for entries.

## Features
* Encrypts entries individually, so if something goes wrong at least it's not the whole vault that fails.
  * This also means the open file dialog wears the hat for 'searching'.
  * Yes, that means that the entry name is not secret since it's used as the filename.
  * Decrypts files individually, so the whole vault is not exposed if program is left running.
  * You'll have to enter the master passphrase each time on open/save.
    * But no worries, I double check that when saving, the same master passphrase was used on a random file to prevent typo issues.
* The 'import' program can parse the csv export of KeePassXC and save all that in my format.
* Multiple front-end
  * GTK 4
    * Beware, this family of libraries has a bad history when it comes to testing before releases.
      * Once in a version, searching in the file dialog would crash this app.
      * Once in a version, going full screen would crash X (not applicable to this app, but hey).
  * Command line
    * Read-only, meant as a emergency fall-back when GTK 4 is unusable.
  * Windows version of command line front-end
  * MSYS2 Windows version of the GTK 4 front-end
  * QT6
    * I wrote this one because the GTK 4 version is really slow (i.e. multiple seconds) at loading the open file dialog now that I'm running KDE Plasma 6 instead of OpenBox.
    * I'm hoping that this will be my way into an android port.

## Future works
* Android version
* I never centralized the encryption related code so it's duplicated in the various GUIS.
  * It would need a lot of work, and it's not thousand of lines so I never bothered.

## Sorry
* It's written in fibrofog C, and the Windows ports were hastily done.