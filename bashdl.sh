#!/bin/bash
#
source 'functions.inc';

if ! check_for_bash; then
    log_error "main" "Not running with a bash shell?!";
    exit 1;
fi;

## Configuration to be used in the script.
#
# copy paste madness from http://wooledge.org/mywiki/BashFAQ/035
# Default options
configuration_file="bashdl.conf";

## If you go below, don't break anything :p

declare verbose="0";
declare quiet="0";
declare folder_tmp="";
declare folder_complete="";
declare yes="0";

_move_complete="0";
_list_modules="0";
_list_config="0";
_last_downloaded_file="";
_override_options="";

function _help() {
   cat <<ENDOFMESSAGE
Usage: $0 [options] <arguments>
Download a file through one of the supplied modules.

    -c, --config FILE   Uses specified file as configuration file.
    -h, --help          Displays this help message and exits.
    -m, --modules       List all hooks and exits.
    -l, --list-config   Lists out the default configuration and exits.
                        You can use this to create a default configuration
                        file by running the following command:
                          '$0 -l > download.conf 2>/dev/null'
    -o, --options OPTS  Override the default options with these.
                        OPTS should be formatted as 'opt=value' and
                        seperated with a comma (',').
    -q, --quiet         Don't print anything. 
                        (Only works for modules that use internal log system.)
    -v, --verbose       Switches verbosity on. 
                        Configuration setting: "verbose=1";
    -y, --yes           Don't ask before starting the download.
    -k, --keep-tmp      Don't remove any temp files or directories
                        (Only if a module doesn't remove them himself.)

The <arguments> can be one or more 'files' or 'urls' or the combination
of both. If a file is encountered, we will look for urls in it and loop
over them.

ENDOFMESSAGE
    exit 0;
}

if `getopt -T >/dev/null 2>&1` ; [ $? = 4 ] ; then
  true; # Enhanced getopt.
else
  log_error "main" "You are using an old getopt version $(getopt -V)";
  exit 1;
fi


TEMP=`getopt -o -yo:qlmhvkc:: --long yes,options:,quiet,list-config,modules,help,verbose,keep-tmp,config:: \
    -n 'bashdl' -- "$@"`;

if [[ $? != 0 ]]; then
    log_error "main" "Terminating...";
    exit 1;
fi;

while [ $# -gt 0 ]
do
    case "$1" in
        -y|--yes)       yes="1";;
        -k|--keep-tmp)  keep_tmp="1";;
        -v|--verbose)   if [[ "$quiet" == "1" ]]; then
                            log_warn "main" "You can not run quiet AND verbose (Already quiet).";
                        else
                            verbose="1";
                        fi;;
        -c|--config)    configuration_file="$2";
                        if [[ ! -f "$configuration_file" ]]; then
                            log_error "main" "\"$configuration_file\" is not a file";
                            exit 1;
                        fi;
                        shift;;
        -h|--help)      _help;;
        -q|--quiet)     if [[ "$verbose" == "1" ]]; then
                            log_warn "main" "You can not run quiet AND verbose (Already verbose).";
                        else
                            quiet="1";
                        fi;;
        -m|--modules)   _list_modules="1";;
        -o|--options)   _override_options="$2";
                        shift;;
        -l|--list-config) quiet="1"; _list_config="1";;
        --)             shift; break;;
        -*)             echo "Command option not recognized";
                    exit 1;;
        *)              break;;
    esac
    shift;
done;



if [[ "$verbose" == "1" ]]; then
    log_info "main" "Turned verbose mode on";
    log_warn "main" "The effect of this switch depends on the implementation of the modules!";
fi;

if [[ -z "$configuration_file" ]]; then
    configuration_file="./bashdl.conf";
fi

if [[ -f "$configuration_file" ]]; then
    source "$configuration_file";
    if [[ -n "$_override_options" ]]; then
        parse_options "$_override_options";
    fi;
else
    log_error "config" "No configuration file found.";
    log_info "config" "Create a new file called $configuration_file with your configuration settings.";
    if [[ "$_list_config" == "1" ]]; then
        true; #
    else
        exit 1;
    fi;
fi;


_config_error="0";
## check global configuration
if [[ -z $folder_tmp ]]; then
    log_warn "config" "Configuration option \"folder_tmp\" is not set. Defaulting to \"./downloads\"";
    folder_tmp="./downloads";
else
    # Hack to append the leading "/" if it doesn't exist already.
    # I should probably also expand the path to make sure it always works. 
    # You never know when a module goes woopie.
    fixDir "folder_tmp";

fi;
## Check if this folder really exists.
if [[ ! -d $folder_tmp ]]; then
    log_error "config" "The configured folder_tmp \"$folder_tmp\" does not exist.";
    _config_error="1";
fi

if [[ -z $folder_complete ]]; then
    folder_complete="$folder_tmp";
    _move_complete="0";
else
    fixDir "folder_complete";
    _move_complete="1";
    if [[ ! -d $folder_complete ]]; then
        log_error "config" "The configured folder folder_complete \"$folder_complete\" does not exist."
        _config_error="1";

    fi;
fi

## Load the modules.
load_modules;

if [[ "$_list_modules" == "1" ]]; then
    regex_map_list;
    exit 0;
fi;

if [[ "$_list_config" == "1" ]]; then
    _list_config;
    exit 0;
fi;

if [[ "$_config_error" == "1" ]]; then
    exit 1;
fi;

if [[ ${#@} == "0" ]]; then
    _help;
fi;


## Collecting all urls.
resultvalue="0";
for arg in "$@"; do
    collect_url "$arg";
    if [[ $? == 1 ]]; then
        resultvalue="1";
    fi;
done;
if [[ "$resultvalue" == "1" ]]; then
    log_info "main" "I was unable to parse all arguments to urls.";
fi;

url_list.list;

do_yes "Ready to start downloading...";
if [[ $? == 0 ]]; then
    url_list.run;
fi;
url_list.list;


trap - INT TERM EXIT;
