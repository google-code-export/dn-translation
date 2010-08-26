Dragon Nest CN Community Translation Project
For client version: v27

Disclaimer: 
All files are provided "as is", use at your own risk!

Project Home:
http://code.google.com/p/dn-translation

# # # # # # # # # #

HOW TO UPDATE

Run SVN-Update.bat to update to latest version.

# # # # # # # # # #

HOW TO TRANSLATE AND TEST

[resource-originals] contains all the untranslated XML files,
once translated, put the translated XML file into the correct 
subfolder inside [resource], then run Pack-???.bat to generate
the translation pack you want to use.

Copy the generated pak file into the game folder and it
should be automatically loaded.

Check out the guides on the project website to help you with
translation tasks. Also remember to check what other people
is working on to prevent duplications. 

# # # # # # # # # #

KNOWN ISSUES

Current limitation is that the packer cannot work with more
than 500 files in a single folder, so do not put any
untranslated files into the [resource] folder.
