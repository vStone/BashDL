#!/bin/bash

## Module information
# Info="Example module. Base your new modules on this one and other";
# Author="Po0ky <Jan@buitendezone.be>";
# Homepage="http://www.buitendezone.be/";

## Example module
# Follow this example to add your own fetchers...

# The name of this module is "example". The file in which this module is
# hosted should be named "<modulename>.mod". Any other files will be ignored
# While loading. You can use this to add extra files to keep your code clean.


## ---------- Prerequisites -----------
# List all external programs you will be using here
example_prerequisites=( "/bin/ls" );

## ---------- Configuration ----------- 
# List configuration options you require here.
# Optional options should be prefixed with "_".
# So, if you need a username and an optional password, use sth like this:
# 
example_config=( "username" "_password" );


## ----------- Dependencies ------------ 
# If your module depends on another module, place it's name here.
# Afterwards you are free to use the functions that are used in it.
# 
# If you use configuration settings from a module you depend on, 
# dont list it in the '<modulename>_config' array. It will result in people 
# having to enter the same information twice.

# For example: if you intend to use any wget functions allready specified in
# the wget module. place it in the '<modulename>_dep' array.
example_dep=( "wget" ); 


## ---------- URL regex -------------
# This is the part that actually hooks your module into BashDL.
# By choosing a regular expression that matches the url that your
# module was designed for, we make sure only these kinds of urls get
# send to your function hooks.
#
# Take good care of the order you place these patterns in.
# The first match will be used to download the url.
# So, put the url that will match the most urls, _last_.
#
# If two modules match the same pattern, we will look for the one
# that has the best match.
# If both matches are equal, the module last in the dependency tree will be used
# (ie the module that extends from the other.
# If neither of the modules depends on the other, an error will be thrown and
# the user will warned to remove one of the modules that conflict with eachother.

# Once you established the regular expression to use, the only thing that lasts is to escape all backslashes that you used. :/ I know, it's a pain, but it has to be done.
 
# normally we would use 'http://www\.example\.com/files/.*';
example_regex[0]='http://www\\.example\\.com/files/.*';
example_hook[0]="download_url";

example_regex[1]='http://www\\.example\\.com/users/.*';
example_hook[1]="download_user";

# Next, create the functions for the hooks. The function name
# MUST ALWAYS BE "<modulename>.<hook>" 
# A hook should take following arguments!
#  url="$1";
#
#  following variables should be set with appropriate values
#  before returning
#  -> _last_downloaded_file


# First argument is the url to be downloaded
# Second argument is the prefix you should if you are logging. 

function example.download_url () {
    local url="$1";

    # Example log line.
    log_module "example" "Downloading url: $url";

    # Create a temp output file.
    # Fail if it fails.
    local tmpfile="/dev/null";
    if maketemp; then
        tmpfile="$tmp_file";
    else
        return 1;
    fi;
    

    # Program with built in output to file.
    wget -o $tmpfile 2>&1 & 
    # Redirect output from program.
    # cat <filename> 2>&1 > $tmpfile &    

    pid=$!;

    # Example following the log file of a process with pid <pid>
    log_file "$pid" "$tmpfile"

    

}

function example.download_user() {
    # Do stuff here
    return 0;

}



## END OF MODULE CONFIGURATION 
# -------------------------------
# I blank out the settings here to make sure this
# example doesn't interfere with the actual working modules
# Don't modify this.
#
example_prerequisites=( );
example_config=( );
example_default=( );
example_dep=( );
