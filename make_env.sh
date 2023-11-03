#!/bin/bash

CURRENT_PATH=$(cd `dirname $0`; pwd)



function info() {
    echo -e "\n=========================================================================="
    echo -e "\e[1;32mPLARFORM:\e[0m			$PLARFORM"
    echo -e "\e[1;32mCONFIG:\e[0m			$CONFIG"
    echo -e "==========================================================================\n"
}


function check_platform_config() {
	config_list=$(ls config)
	echo -e "\033[33;1mPlease Choose one from below for platform\033[0m\n"
	index=0
	for p in $config_list;
	do
		if [[ 'inc' != $p && 'inc_porting' != $p && 'src' != $p && 'inc_hide' != $p ]]; then
			let index++
			array[$index]=$p
			echo -e "\t\033[32;1m[$index] $p\033[0m"
		fi
	done
	echo -en "\n\033[33;1mInput config num (1-${#array[*]}): \033[0m"
	read -e NUM
	if [[ $NUM -gt ${#array[*]} || $NUM -le 0 ]];then
		echo -en "\n\033[31;1mInput platform num out of range!! \033[0m\n"
		exit 1
	fi
	PLARFORM=${array[$NUM]}

	echo -e "Choose platform = \033[31;1m$PLARFORM\033[0m"
}

function check_compile_config() {
	config_list=$(ls config/$PLARFORM)
	echo -e "\033[33;1mPlease Choose one from below for compile config\033[0m\n"
	index=0
	for p in $config_list;
	do
		let index++
		array[$index]=$p
		echo -e "\t\033[32;1m[$index] $p\033[0m"
	done
	echo -en "\n\033[33;1mInput config num (1-${#array[*]}): \033[0m"
	read -e NUM
	if [[ $NUM -gt ${#array[*]} || $NUM -le 0 ]];then
		echo -en "\n\033[31;1mInput compile num out of range!! \033[0m\n"
		exit 1
	fi
	CONFIG=${array[$NUM]}

	echo -e "Choose compile config = \033[31;1m$CHIPSET\033[0m"
}



check_platform_config
check_compile_config

info

cp config/$PLARFORM/$CONFIG .config -f

echo -e "\033[31;1mSucess make config\033[0m"
