
## 
# Module Management.
#
##
# Holds all loaded modules.
declare -a loaded_modules=();


function loaded_modules_add() {
    local name=$*;
    local index=${#loaded_modules[@]};
    log_debug modules "Adding to loaded modules: $index -> $name";
    loaded_modules[$index]=$name;
};

function module_check_prereq() {
    local name=$*;
    local reqname="${name}_prerequisites[@]";
    local -a prereq_arr="( ${!reqname} )";
    local element="";
    log_debug "modules" "Checking requirements for $name";
    for ((i=0; $i < ${#prereq_arr[@]}; i=$i+1)); do
        element=${prereq_arr[$i]};
        log_debug "modules" "  Checking $element (required by $name)";
        if [[ ! -f $element ]]; then
            tmpelement=( $( whereis -b $element | sed "s|$element:\(.*\)|\1|" ) );
            tmpelement="${tmpelement[0]}";
            if [[ -n $tmpelement ]]; then
                element="$tmpelement";
            fi;
        fi;
        if [[ ! -x $element ]]; then
            log_error "modules" "  Missing $element for module $name";
            return 1;
        fi;
    done;
    return 0;
}


## Check dependencies for a module.
# WARNING (BAD FORMATTED MODULES WILL PASS THIS TEST!!" (missing '*_dep' array);

function module_check_dep() {
    # Get the dependency array for the module.
    local name="$1";
    local depname="${name}_dep[@]";
    local -a dep_arr="( ${!depname} )";
    local dependency="";
    for ((i=0; $i < ${#dep_arr[@]}; i=$i+1)); do
        dependency=${dep_arr[$i]};
        local found="0";
        for mod in ${loaded_modules[@]}; do 
            if [[ "$mod" == "$dependency" ]]; then 
                found="1"
                break;
            fi;
        done;
        if [[ $found == "0" ]]; then
            return 1;
        fi;
    done;
    return 0;
}


function module_check_config() {
    # Get the dependency array for the module.
    local name="$1";
    local confname="${name}_config[@]";
    local -a conf_arr="( ${!confname} )";
    local confstring="";
    local returnval="0";
    for ((i=0; $i < ${#conf_arr[@]}; i=$i+1)); do
        confstring="${conf_arr[$i]}";
        local optional="0";
        if [[ "${confstring:0:1}" == "_" ]]; then
            confstring="${confstring:1}";
            optional="1";
        fi;
        confstring="${name}_${confstring}";
        if [[ -z ${!confstring} ]]; then
            local msg="  Missing configuration string \"$confstring\"";
            if [[ "$optional" == "1" ]]; then
                log_warn "module_check_config" "$msg";
            else
                log_error "module_check_config" "$msg";
                returnval="1"
            fi;
        fi;
    done;
    return $returnval;

}

function check_module() {
    local name="$1";
    log_debug "modules" "Checking module: $name";
    module_check_prereq $name || return 1;
    module_check_config $name || return 1;
}

function hook_module() {
    local name="$1";
    local regexname="${name}_regex[@]";
    local hookname="${name}_hook[@]";
    local -a regex_arr="( ${!regexname} )";
    local -a hook_arr="( ${!hookname} )";
    local regexstring="";
    local hookstring="";
    for ((i=0; $i < ${#regex_arr[@]}; i=$i+1)); do
        regexstring="${regex_arr[$i]}";
        hookstring="${hook_arr[$i]}";
        if function_exists "${name}.${hookstring}"; then
            true; # Exists
        else
            log_error "hook_module" "Hook \"$hookstring\" not defined in module $name!";
            return 1;
        fi;

       # log_info "hook_module" "Hooking $hookstring for \"$regexstring\"";
        if [[ -z $regexstring || -z $hookstring ]]; then
            log_error "hook_module" "No hook ?!";
            return 1;
        else
            if regex_map_add "$name" "$hookstring" "$regexstring"; then
                true; # Hooked!
            else
                log_error "hook_module" "Failed to a the hook for module $name";
                log_error "hook_module" " Could not add: \"$regexstring\" -> \"$hookstring\"";
                return 1;
            fi;
        fi;
    done;
    return 0;
}

function load_module() {
    local name="$1";
    local prefix="Loading module [$name]";
#    log_info "load_module" "$prefix Checking prerequisites & configuration";
    if check_module "$name"; then
#        log_info "load_module" "$prefix Checks passed." "$PREVIOUS";
        true;
    else
        log_error "load_modules" "Checks failed on $name";
        ## Fixing a bug with -l if configuration is not specified :/
        # _list_config
        if [[ "$_list_config" == "1" ]]; then
            log_warn "load_modules" "Configuration missing! But.. since you probably want to create it, let's continue.";
            loaded_modules_add "$name";
            return 0;
        else
            exit 1;
        fi;
    fi;
    if hook_module "$name"; then
       log_debug "load_modules" "Hooks registered";
        true;
    else 
        log_error "load_modules" "Could not hook module";
        exit 1;
    fi;
    log_info "load_module" "";
    log_info "load_module" "$prefix\t\t[${BGREEN}OK$RESET]" "$PREVIOUS";
    loaded_modules_add "$name";
    return 0;
}

function load_modules() {
    log_info "load_modules" "Loading modules...";

    local index="0";
    local -a modules;
    # Loop over all the files in the modules.d directory and add them to the 'modules' array.
    for file in ./modules.d/*.mod; do
        name=$( echo $file | sed "s|\./modules\.d/\(.*\)\.mod|\1|" );
        modules[$index]=$name;
        source "$file";
        let "index++";
    done;


    # Loop over all modules. If there are dependency issues, add it to the retry list.
    #
    local status="1";
    # Initial: all modules have to be loaded.
    local -a loading=( "${modules[@]}" );
    local -a retry_modules=();

    while [[ $status > "0" ]]; do
        # Keep count how many modules we load. In a case of circular dependencies,
        # this will tell us to break from it.
        local loadcount="0";
        # Initial: No modules in the retry list.
        retry_modules=();


        # Loop over the modules and check the dependencies.
        for (( j=0; $j < ${#loading[@]}; j=$j+1 )); do
            element="${loading[$j]}";
            # If the module has all required dependencies loaded, we can 'load_module' it.
            if module_check_dep $element; then
                # ONLY if the module was successfully loaded, 
                # we increase the loadcount for this run (this loop over the 'loading' array).
                if load_module $element; then
                    let "loadcount++";
                fi

            else
                # We are still missing dependencies for this module. Try it again later.
                local index=${#retry_modules[@]};
                retry_modules[$index]=$element;
            fi;
        done;


        # If there is not one module loaded during the checkout, we have a dependency problem.
        if [[ $loadcount == "0" ]]; then
             log_error "load_modules" "Module dependency issues!!";
             log_error "load_modules" "Check following modules for errors:";
             for ((j=0; $j < ${#loading[@]}; j=$j+1)); do element=${loading[$j]};  depname="${element}_dep[@]"; log_error "load_modules" "   $GREEN$element$RESET  \t-> deps ($GREEN ${!depname} $RESET)";  done;
             return 1;
        fi;

        # If the number of modules that we have to retry is 0, we are finished!
        # otherwise, we have to keep going and try them again.
        local count="${#retry_modules[@]}";
        if [[ $count > 0 ]]; then
            loading=();
            loading=( "${retry_modules[@]}" );
        else 
            status="0";
        fi;
    done;

    return 0;
}

function _list_module_config() {

    for ((i=0; $i < ${#loaded_modules[@]}; i=$i+1)); do
        local modulename="${loaded_modules[$i]}";
        printf "## %s\n" "$modulename";
        local confname="${modulename}_config[@]";
        local defname="${modulename}_default[@]";
        local -a conf_arr="( ${!confname} )";
        local -a def_arr="( ${!defname} )";

        for ((j=0; $j < ${#conf_arr[@]}; j=$j+1)); do
            confstring="${conf_arr[$j]}";
            defstring="${def_arr[$j]}";
            if [[ $defstring == "_" ]]; then
                defstring="";
            fi;
            local optional="0";
            if [[ "${confstring:0:1}" == "_" ]]; then
                confstring="${confstring:1}";
                optional="1";
            fi;
            confstring="${modulename}_${confstring}";
            if [[ "$optional" == "1" ]]; then
                printf "#%s=\"%s\"\n" "$confstring" "$defstring";
            else
                printf "%s=\"%s\"\n" "$confstring" "$defstring";
            fi;
        done;
        printf "\n";
    done;
}
