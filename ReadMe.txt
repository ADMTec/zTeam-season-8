Changes:

- All existing Community Fixes
- zGS crash when runing on VPS Fixed
- Removed some logs only informational and not really important which only fill much space
- Fixed 3 crash bugs about values divided by zero(they happen on very rare situations only,
i've saw them max 1-2 times each for 3 months) (itemsetopion/elementalsystem)
- Improvements on AntiHack
- Improvements on Log System (Most important logs are separated to can be track more easily)
- Added New logs (AntiHack/PrivateChat/Post/GMSystem/USystem)(Only most important stuff some
commands still not have logs)
- Added New Commands for GM's/Users
- Exp 0/0 Fix(If level experience is bigger than next level experience it goes to 0(solution good
only for fun servers.....))
- 2 Guild Crashes Fixed(Server and Client)
- Acheron Guardian Event when the event End move everyone to Noria + no one cant open NPC anymore
is Fixed
- Doppleganger the last box not open sometimes Fixed
- Blue and Summon the Demons Event Added
- GM/Admin announce message on Login Added
- GM name on global Chat ! Added
- Duel Announce on Global Chat + Score Added
- Normal Chat/Post/Vault flood time protection Added
- Arena is NoN-PVP Map(you can change from source to whatever map you want) Added
- SD Bug Fixed
- Potion delay time between each potion Added(Fully customizable)
- GM shop(Cant be used except CtlCode=32)(Kalima NPC) Added
- All Pentagrams drop Added
- Elemental System all elements only Fire changed to it's proper element Fixed
- ArcaBattle removed the check for party only with your Guild
- Client uses less memory patch Added
It could have and other stuff i don't remember them all :(

Problems:

- Sometimes the connection between zMS<>zGS is lost or something other but with few words.....:
The zMS not send correct information to zGS and in zGS Expanded Inventory size is more than 4
or less than 1 which results into client Crash :(
Or it goes to 0 even if you have in DB 4 as size so you cant use your Expanded Inventory anymore.
This happens on very rare situations!!!!!
Temporary i've maked if Expanded Inventory Size is more than 4 to be exactly 4 and if is 0 to
get to 4.
If you want you can edit and use only the > 4 = 4 option.
Custom debug log is added after each character login to identify the value of Expanded Inventory,
can be shutdown if is used option > 4 = 4 and 0,1,2,3,4 = 4.
- Sometimes again becouse bad connectivity between zMS<>zGS Elemental Item information is lost :(

Bugs:

- All existing in other packs

Notes:

- Visual Studio 2010 Professional used for compile project
- Visual Studio Zen Coding (https://visualstudiogallery.msdn.microsoft.com/924090a6-1177-4e81-96a3-e929d7193130)
needed to compile project
- If you get some errors on compiling project it might be caused becouse /crypto/, re-compile
crypto and try again

Requirements:

- Microsoft SQL Server 2008 R2 x86/x64 Standard Edition and upper



Credits:
WebZen
zTeam
Omaru
michi28
jackbot
Ashlay
Natzugen
Soumyxorp
tiagoassis
Mentor
daimon
Smiley
Mr.Haziel
InFamous
Shatter
MuServerEx
Deathway
and whole MuOnline Community :)
