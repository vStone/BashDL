#!/bin/bash
#

## Beautify the output a bit? :)
#  You can clear these out if you don't like colors... asshole :p

RED="\033[31m";         GREEN="\033[32m";           YELLOW="\033[33m";
BLUE="\033[34m";        MAGENTA="\033[35m";         CYAN="\033[36m";
WHITE="\033[37m";       DEFAULT="\033[39m";

RESET="\033[0m";        BOLD="\033[1m";             ITALIC="\033[23m";
UNDERLINE="\033[24m";   INVERSE="\033[27m";         STRIKETHROUGH="\033[29";
BRED="$BOLD$RED";       BGREEN="$BOLD$GREEN";       BYELLOW="$BOLD$YELLOW";
BBLUE="$BOLD$BLUE";     BMAGENTA="$BOLD$MAGENTA";   BCYAN="$BOLD$CYAN";


PREVIOUS="\033[1A\033[K"; # Go to previous line (Remove this, and you will get loads of spam...)

declare -a running_pids=( );
declare -a tmp_files=( );

function add_pid() {
    local pid=$1;
    local pidcount="${#running_pids[@]}";
    running_pids[$pidcount]="$pid";
    log_debug "add_pid" "Added pid to the list: $pid";
}

function del_pid() {
    local pid=$1;
    local element;
    local -a clean_pids=();
    local counter="0";
    for (( x=0; $x < ${#running_pids[@]}; x=$x+1 )); do
        element="${running_pids[$x]}";
        if [[ "$element" -eq "$pid" ]]; then
            log_debug "del_pid" "Removed pid $pid from the list";
        else
            clean_pids[$counter]="$element";
            let "counter++";
        fi
    done;
    running_pids=();
    running_pids="${clean_pids[@]}";
}


function cleanup() {
  if [[ ${#running_pids[@]} > 0 ]]; then
    log_info "cleanup" "Cleaning up. Shutting down running processes.";

    local element;
    for (( x=0; $x < ${#running_pids[$x]}; x=$x+1 )); do
        element="${running_pids[$x]}";
        kill $element;
    done;
  fi;
  if [[ $keep_tmp == 1 ]]; then
    log_info "cleanup" "Keeping all temporary files and/or directories.";
  else
    log_info "cleanup" "Removing all temporary files and/or directories.";
    for (( x=0; $x < ${#tmp_files[$x]}; x=$x+1 )); do
        element="${tmp_files[$x]}";
        if [[ -e $element ]]; then
          log_info "cleanup" "$(rm -vr $element)";
        fi;
    done;
  fi;
}

declare abort="0";
declare trap_abort="trap abort INT TERM EXIT";

eval $trap_abort;
## Unset before manual exit.

function abort() {
    if [[ $abort == 0 ]]; then
        abort="1";
        log_debug "abort" "Abort trapped.";

        if [[ ${#running_pids[@]} > 0 ]]; then
          log_info "abort" "List of running processes.";
          log_command "log_info" "abort" "ps -o pid,state,start,command,user,group,pmem --pid ${running_pids[@]}";

          do_yes "Are you sure you want to shut down all running processes?";
          if [[ $? == 0 ]]; then
              cleanup;
              exit;
          else
            eval $trap_abort;
            abort="0";
          fi;
        else
            cleanup;
            exit;
        fi;
    else
        log_debug "abort" "Already aborting";
    fi;

}



declare tmp_file="";
declare tmp_dir="";

function maketempd() {
    local name=$1;
    if [[  ${#name} == 0 ]]; then
        name="bashdl";
    fi;
    log_debug "maketempd" "Trying to create $name folder in $folder_tmp";
    local tmpdir="";
          tmpdir=`mktemp -t -d -p "$folder_tmp" "$name.XXXXXX" 2>&1`;
    if [[ ! $? == 0 ]]; then
        log_error "maketemp" "I was unable to create a directory. [Error: $tmpdir]";
        return 1;
    fi;
    local tmpcount="${#tmp_files[@]}";
    tmp_files[$tmpcount]="$tmpdir";
    tmp_dir="$tmpdir";
    return 0;
}

function maketemp() {
    local name=$1;
    if [[ ${#name} == 0 ]]; then
        name="bashdl"
    fi;
    log_debug "maketemp" "Trying to create a file $name in $folder_tmp";
    local tmpfile="";
          tmpfile=`mktemp -t -p $folder_tmp "$name.XXXXXX" 2>&1`;
    if [[ ! $? == 0 ]]; then
        log_error "maketemp" "I was unable to create a file. [Error: $tmpfile]";
        return 1;
    fi;
    log_debug "maketemp" "Created temp file: $tmpfile";
    local tmpcount="${#tmp_files[@]}";
    tmp_files[$tmpcount]="$tmpfile";
    tmp_file="$tmpfile";
    return 0;
}

## check_for_bash
#  Returns '0' if the script is running under bash
#  Returns '1' if it doesn't.
function check_for_bash() {
  pid=$$;
  if [[ -f "/proc/${pid}/cmdline" ]]; then
    if [[ -z $( cat "/proc/${pid}/cmdline" | sed "s|$0||g" | grep "bash" ) ]]; then
      return 1;
    else
      return 0;
    fi;
  fi;
}


function do_yes() {
    local q="$quiet";
    local message="$1";
    if [[ ! $yes == 1 ]]; then
        quiet="0";
        log_info "do_yes" "$message";
        log_info "do_yes" "--> Press ENTER to continue. (Type \"No\" to cancel)";
        quiet="$q";
        read -s pause;
        if [[ $pause == "No" || $pause == "no" || $pause == "n" ]]; then
          return 1;
        fi;
          return 0;
    fi;
}

function function_exists() {
    local function_name="$1";
    if [[ $( declare -F "$function_name") == "$function_name" ]]; then
        return 0;
    else
        return 1;
    fi
}


function print_array() {
    local name=$1;
    local arrname="${name}[@]";
    local -a arr="( ${!arrname} )";
    log_debug "printarr" "  ---------------------------------";
    log_debug "printarr" "   Printing $name";
    log_debug "printarr" "  ---------------------------------";

    local element;
    for (( x=0; $x < ${#arr[@]}; x=$x+1 )); do
        element="${arr[$x]}";
        log_debug "printarr" "       [$x] => [$element]";
    done;
    log_debug "printarr" "  ---------------------------------";

}

function run_hook() {
    local hook="$1";
    local arg="$2";
    log_debug "run_hook" "Trying to run hook \"$hook\"";
    if declare -F $hook >/dev/null 2>&1; then
        log_debug "run_hook" "Function \"$hook\" exists. Running...";
    else
        log_error "run_hook" "Hook \"$hook\" does not exist!";
        return 1;
    fi;

#    local curdir="$PWD";
#    cd "$folder_tmp";
    $hook "$arg";
    local retval=$?;

#    cd "$curdir";



    if [[ "$retval" == 127 ]]; then
        log_error "run_hook" "Unable to run hook. No such command!";
        return 1;
    elif [[ "$retval" == 1 ]]; then
        log_debug "run_hook" "Running hook failed";
        return 1;
    elif [[ "$retval" == 0 ]]; then
        log_debug "run_hook" "Hook ran well";
        process_file;
        return 0;
    else
        log_warn "run_hook" "Running hook failed? (Unknown return value)";
        return 1;
    fi;

}


function process_file() {
    local fd="$_last_downloaded_file";
    local fdb=$( basename $fd );

    log_debug "process_file" "Processing file: $fd";
    if [[ -a $fd ]]; then
        if [[ $_move_complete == 1 ]]; then
            if [[ $yes == 1 ]]; then
                local count="0";
                local fdbn="$fdb";
                while [[ -a "$folder_complete/$fdbn" ]]; do
                    let "count++";
                    fdbn="$fdb.$count";
                done;
                fdb="$fdbn";
                log_debug "process_file" "$( mv -v $fd $folder_complete/$fdb 2>&1 )";
            else
                local count="0";
                local fdbn="$fdb";
                while [[ -a $folder_complete/$fdbn ]]; do
                    let "count++";
                    fdbn="$fdb.$count"
                done;
                if [[ -a $folder_complete/$fdb ]]; then
                    if question "Overwrite the existing file"; then
                        log_info "process_file" "$( mv -v $fd $folder_complete/$fdb )";
                    else
                        log_info "process_file" "$( mv -v $fd $folder_complete/$fdbn )";
                    fi;
                else
                    log_info "process_file" "$( mv -v $fd $folder_complete/$fdb )";
                fi;
            fi;
        fi;
    else
        log_warn "process_file" "Failed to move file. Source not found!";
    fi;
}

function question() {
    local msg="$1";
    local answered="0";
    while [[ $answered == 0 ]]; do
        _log "INFO" "$BGREEN" "question" "$msg? [ Yes | No ] ";
        echo -ne "$_log_formatted";
        read answer;
        if [[ -z $answer ]]; then
            return 0;
        elif [[ -n $( echo $answer | grep -o "^[yesYES][^noNO]*$") ]]; then
            return 0;
        elif [[ -n $( echo $answer | grep -o "^[noNO][^yesYES]*$") ]]; then
            return 1;
        fi;
    done;

}

function array_key_exists() {
    local keyname="$1";
    shift;
    local -a arrayke=( "$@" );
    for ((b=0; b < ${#arrayke[@]}; b=$b+1)); do
        local key="${arrayke[$b]}";
        if [[ "$key" == "$keyname" ]]; then
            return 0;
        fi;
    done;
    return 1;
}



function _list_config_opt() {
    setting="$1";
    default="$2";
    optional="$3";
    title="$4";
    description="$5";

    if [[ -n $title ]]; then  printf "## %s\n" "$title"; fi;
    while read line; do
        if [[ -n "$line" ]]; then 
            printf "# %s\n" "$line";
        fi;
    done < <(echo "$description");
    if [[ "$optional" == "1" ]]; then
        printf "#%s=\"%s\"\n\n" "$setting" "$default";
    else
        printf "%s=\"%s\"\n\n" "$setting" "$default";
    fi;
}


function _list_config() {
    printf "## This is the default configuration.\n";
    printf "#  Required settings without a default value will\n";
    printf "#   be set to an empty string.\n";
    printf "#  Optional values will be commented out.\n\n";

##
    _list_config_opt "verbose" "0" "1" "Be verbose" \
"Prints debug information.
Can be useful when writing a module.
The effect may differ depending on the module that is used.";
##
    _list_config_opt "quiet"   "0" "1" "Be quiet" \
"Prints almost nothing.
The effect may differ depending on the module that is used.";
##
    _list_config_opt "folder_tmp" "./downloads" "0" "Download folder" \
"Folder to store files while they are being downloaded.
If \"folder_complete\" is not specified,\nfinished files stay here too.
(If you forget the trailing '/', the script will deal with it.)";
##
    _list_config_opt "folder_complete" "" "1" "Completed folder." \
"Folder where the files are to be moved to
after a successful download";

    _list_config_opt "yes" "1" "1" "Yes always!" \
"Ask for permission before starting the downloads
Set to '1' to enable. Disabled by default.";

    _list_config_opt "log_format" "$log_format" "1" "Log formatting" \
"log format";
    _list_config_opt "log_format_verbose" "$log_format_verbose" "1" "" "";
    _list_config_opt "log_date_format" "" "1" "" "";
    printf "## ---------- Modules ----------\n";
    _list_module_config;


    printf "## End of the configuration. You're done!\n";
}

function parse_options() {
    log_debug "parse_options" "Parsing options: \"$1\"";
    local oldifs="$IFS";
    IFS=",";
    local -a arr=( $1 );
    local part opt value decl;
    for ((a=0; a < ${#arr[@]}; a=$a+1)); do
        part=${arr[$a]};
        opt=${part%%=*};
        log_debug "parse_options" "Option: $opt";
        value=${part#*=};
        log_debug "parse_options" "Value:  $value";
        decl="$opt=\"$value\"";
        log_debug "parse_options" "Declaration: $decl";
        log_debug "parse_options" "Overriding option: \"$opt\" with value \"$value\"";
        eval "$opt=\"$value\"";
    done;

    IFS="$oldifs";
}

function fixDir() {
    local varname="$1";
    local value="${!varname}"
    local count="${#value[*]}";
    local eval;
    # Append trailing "/" to the directory.
    local last;
    let "last = count -1";
    if [[ ! "${value:$last}" == "/" ]]; then
        value="${value}/";
    fi;

    eval "$varname=\"${value}\"" >/dev/null 2>&1;
    return $?;

}




source "functions.log.inc" || exit 1;
source "functions.regex_map.inc" || exit 1;
source "functions.modules.inc" || exit 1;
source "functions.url_list.inc" || exit 1;

