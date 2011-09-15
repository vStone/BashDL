#!/bin/bash
#


declare REGEX_IND="3";
declare REGEX_EQUAL="0";
declare REGEX_MORE="1";
declare REGEX_LESS="2";
WHITE="\033[37m"; RESET="\033[0m";

##
#
#  This function compares 2 regexes.
#  return values:
#  REGEX_IND(ependant) -> Regular expressions have nothing to do with eachother.
#  REGEX_EQUAL         -> Identical expressions. (match eachother!)
#  REGEX_MORE          -> First regex is more precise than the second
#  REGEX_LESS          -> First regex is less precise than the second (more matches)
function regex_compareto {
  local reg_one="$1";
  local reg_two="$2";
  local one_match_two=$( echo "$reg_two" | grep -o  --color=no "$reg_one" );
  local two_match_one=$( echo "$reg_one" | grep -o  --color=no "$reg_two" );

  if [[ -z $one_match_two && -z $two_match_one ]]; then
    return $REGEX_IND;
  elif [[ -n $one_match_two && -n $two_match_one ]]; then
    return $REGEX_EQUAL;
  elif [[ -z $one_match_two && -n $two_match_one ]]; then
    return $REGEX_MORE;
  else
    return $REGEX_LESS;
  fi;

}
## Global variables to keep information in.

declare -a arr_regex=( );
declare -a arr_module=( );
declare -a arr_hook=( );
declare -a arr_order=( );



declare lastmatch="";
## This function will be used to find out what module and hook we should use
#  to get this url. It will set the 'lastmatch' variable to the hook we need
#  and return 0 if he did so, otherwise the function returns 1;
function regex_map_match {
    # Clear out the last match.
    lastmatch="";
    # Some variables we will be using
    local url="$1"; local hook=""; local name=""; 
    local regex=""; local index="";
    
    # Loop over all regular expressions in 'arr_order'.
    for ((z=0; $z < ${#arr_order[@]}; z=$z+1)); do
      index="${arr_order[$z]}";
      name="${arr_module[$index]}";
      regex="${arr_regex[$index]}";
      hook="${arr_hook[$index]}";
      match=$( echo "$url" | grep -o "$regex" );
      if [[ -n $match ]]; then
        lastmatch="${name}.${hook}";
        return 0; # Hook found, return and break from this loop.
      fi;
    done;
    return 1; # Nothing found :'(
}

function regex_map_list {
    echo "Listing current modules and hooks";
    local name;
    local hook;
    local regex;
    local index;
    local lines="Index|Module|Hook|Regex";
  lines="$lines\n-----|------|----|-----";
    local newline="";
    for ((z=0; $z < ${#arr_order[@]}; z=$z+1)); do
      index="${arr_order[$z]}";
      name="${arr_module[$index]}";
      regex="${arr_regex[$index]}";
      hook="${arr_hook[$index]}";
      newline="[$z] $index |$name|$hook|$regex";
      lines="${lines}\n${newline}\n";
    done;
    echo -e $lines | column  -t -s "|";


}

function regex_map_add {
    local name="$1";
    local hook="$2";
    local regex="$3";

    local placeindex="0";
    for ((z=0; $z < ${#arr_order[@]}; z=$z+1)); do
        local index="${arr_order[$z]}";
        tmpregex="${arr_regex[$index]}";
        regex_compareto "$regex" "$tmpregex";
        result=$?;
        if [[ $result == $REGEX_IND ]]; then
            true;
        elif [[ $result == $REGEX_EQUAL ]]; then
            return 1;
        elif [[ $result == $REGEX_MORE ]]; then
            break;
        fi;

        let "placeindex++";
    done;

    # Adding with the "current index";
    local count=${#arr_module[@]};
    arr_module[$count]="$name";
    arr_regex[$count]="$regex";
    arr_hook[$count]="$hook";
    arr_order=( ${arr_order[@]:0:$placeindex} $count ${arr_order[@]:$placeindex});
    return 0;
}

