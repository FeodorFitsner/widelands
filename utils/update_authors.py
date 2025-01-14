#!/usr/bin/env python
# encoding: utf-8

import codecs
import json
import os.path
import sys

# This script collects translator credits and locale information
# from the JSON files in ../txts/translators.
# It then collects all other contributors from ../txts/developers.json,
# adds the translators at the hook "Translators"
# and writes the translator and developer credits to ./txts/developers.lua
# The locale information is written to ../data/i18n/locales.lua.

base_path = os.path.abspath(os.path.join(os.path.dirname(__file__),os.path.pardir))

print("Reading locales from JSON:")

source_path = os.path.normpath(base_path + "/data/i18n/locales")

if (not os.path.isdir(source_path)):
	print("Error: Path " + source_path + " not found.")
	sys.exit(1)

# Each language's translators live in a separate file, so we list the dir
source_files = sorted(os.listdir(source_path), key=str.lower)

lua_translators = ""

lua_locales = "-- This file is generated by utils/update_authors.py.\n"
lua_locales += "-- The locale data is managed in Transifex.\n\n"
lua_locales += "return {\n"
lua_locales += "\t-- Locales are identified by their ISO code.\n"
lua_locales += ' \ten = {\n'
lua_locales += "\t\t-- Used to display the locale in the Options menu.\n"
lua_locales += '\t\tname = "English",\n\n'
lua_locales += "\t\t-- Defines the language's position on the list in the Options menu.\n"
lua_locales += '\t\tsort_name = "English",\n\n'
lua_locales += "\t\t-- The font set used, including the script's direction. See i18n/fonts.lua\n"
lua_locales += '\t\tfont = "default"\n'
lua_locales += '\t},\n'

for source_filename in source_files:
	# Only json files, and not the template file please
	if source_filename.endswith(".json") and source_filename != "locales_translators.json":
		source_file = open(source_path + "/" + source_filename, "r")
		translators = json.load(source_file)
		locale_message = "- Added"

		# Parsing translator credits
		# Make sure we don't pick up untranslated stuff
		if translators["translator-list"] != 'translator-credits':
			locale_message += " translators and"
			lua_translators += '{' # entry
			lua_translators += 'subheading = "' + translators["your-language-name"]
			if translators["your-language-name-in-english"] != 'English' and translators["your-language-name-in-english"] != translators["your-language-name"] :
				lua_translators += ' (' + translators["your-language-name-in-english"] + ')'
			lua_translators += '",'

			lua_translators += 'members = {' # members
			for transl_name in translators["translator-list"].split("\n"):
				lua_translators += '"' + transl_name + '",'
			lua_translators += "}," # members
			lua_translators += "}," # entry

		# Parsing locale info
		# Make sure we don't pick up untranslated stuff
		locale_code = source_filename.split(".json")[0]
		locale_message += " locale info for " + locale_code
		lua_locales += '\n\t' + locale_code + ' = {\n' # entry with locale code

		if translators["your-language-name"] != 'English' or locale_code == 'en':
			lua_locales += '\t\tname = "' + translators["your-language-name"] + '",\n'
		else:
			lua_locales += '\t\tname = "' + locale_code + '",\n'

		if translators["language-sort-name"] != 'English' or locale_code == 'en':
			lua_locales += '\t\tsort_name = "' + translators["language-sort-name"] + '",\n'
		else:
			lua_locales += '\t\tsort_name = "' + locale_code + '",\n'

		lua_locales += '\t\tfont = "' + translators["font-set"] + '"\n'
		lua_locales += "\t},\n" # entry
		print(locale_message)
lua_locales += "}\n"

print("Writing locales\n")
dest_filename = "locales.lua"
dest_filepath = os.path.normpath(base_path + "/data/i18n") + "/" + dest_filename
dest_file = codecs.open(dest_filepath, encoding='utf-8', mode='w')
dest_file.write(lua_locales)

print("Reading developers from JSON")
source_path = os.path.normpath(base_path + "/data/txts")

if (not os.path.isdir(source_path)):
	print("Error: Path " + source_path + " not found.")
	sys.exit(1)

source_file = open(source_path + "/developers.json", "r")
developers = json.load(source_file)["developers"]

lua_string = """-- Do not edit this file - it is automatically generated
-- by utils/update_authors.py from developers.json.
"""
lua_string += "function developers() return {" # developers

for category in developers:
	print("- Adding " + category["heading"])
	lua_string += '{' # category
	lua_string += 'heading = _"' + category["heading"] + '",' # This will be localized
	lua_string += 'image = "' + category["image"] + '",'

	lua_string += 'entries = {' # entries
	if category["heading"] == "Translators": # Hook for adding the translators parsed above
		lua_string += lua_translators
	else:
		for subcategory in category["entries"]:
			lua_string += '{' # entry
			if 'subheading' in subcategory:
				lua_string += 'subheading = _"' + subcategory["subheading"] + '",' # This will be localized

			lua_string += 'members = {' # members
			for member in subcategory["members"]:
				lua_string += '"' + member + '",'
			lua_string += "}," # members

			lua_string += "}," # entry
	lua_string += "}," # entries

	lua_string += "}," # category
lua_string += "} end" # developers

print("Writing developers")
dest_filename = "developers.lua"
dest_filepath = source_path + "/" + dest_filename
dest_file = codecs.open(dest_filepath, encoding='utf-8', mode='w')
dest_file.write(lua_string)

print("Fixing the formatting")
import fix_lua_tabs
fix_lua_tabs.main()
print("Done.")
