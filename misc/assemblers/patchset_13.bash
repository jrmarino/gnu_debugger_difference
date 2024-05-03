#!/raven/bin/bash

GDBVERSION=13.2
BUGDIFF=/home/marino/github/gnu_debugger_difference/v13
EXPANSE=/home/marino/GDB-SOURCE
DIFFPROG=/usr/bin/diff
GREPPROG=/usr/bin/grep
PACHPROG=/usr/bin/patch
OUTPUT_DIR=${EXPANSE}/patches-${GDBVERSION}
RELEASE_DIR=${EXPANSE}/gdb-${GDBVERSION}
SCRATCH_DIR=${EXPANSE}/scratch
GENERATED_DIR=${BUGDIFF}/../generated/patches-${GDBVERSION}
MAIN_SUFFIX=gdb
REST_SUFFIX=rest

function reset_patch () {
   PATCH_SUFFIX=${1}
   PATCH_FILE=${OUTPUT_DIR}/diff-${PATCH_SUFFIX}
   PATCH_LINK=${OUTPUT_DIR}/patch-diff-${PATCH_SUFFIX}
   cd ${BUGDIFF}
   rm -f ${PATCH_FILE} ${PATCH_LINK}
   ln -s diff-${PATCH_SUFFIX} ${PATCH_LINK}
}

function produce_patch () {
   PATCH_SUFFIX=${1}
   PATCH_FILE=${OUTPUT_DIR}/diff-${PATCH_SUFFIX}
   declare -a DIRECTORY_LIST=("${!2}")

   cd ${BUGDIFF}
   rm -f ${PATCH_FILE}
   for DIR in ${DIRECTORY_LIST[@]}; do
      echo "     Searching ${DIR}"
      FILES=${DIR}/*
      for F in ${FILES}; do
         if [ -f ${F} ]; then
            if [ -f ${RELEASE_DIR}/${F} ]; then
               DFMSG=`${DIFFPROG} -q ${RELEASE_DIR}/${F} ${F}`
               if [ -n "${DFMSG}" ] ; then
                  echo "diff ${F}"
                  ${DIFFPROG} -u ${RELEASE_DIR}/${F} ${F} --label=${F}.orig --label=${F} >> ${PATCH_FILE}
               fi
            else
               ${DIFFPROG} -u /dev/null ${F} --label=/dev/null --label=${F} >> ${PATCH_FILE}
            fi
         fi
      done
   done
}

function regenerate_patch () {
   PATCH_SUFFIX=${1}
   FLUX_NAME=${2}
   PATCH_FILE=${OUTPUT_DIR}/diff-${PATCH_SUFFIX}
   FLUX_PATCH=${BUGDIFF}/../misc/flux_patches_v13/${FLUX_NAME}
   AWKCMD='{if (substr($2,0,2) == "b/") print substr($2,3); else print $2}'
   AWKCM2='NR==1 {if (substr($2,0,2) == "b/") print "-p1"}'

   cd ${SCRATCH_DIR}
   IFS=$'\n'
   FILE_LIST=`${GREPPROG} '^+++ ' ${FLUX_PATCH}`
   for FILE in ${FILE_LIST[@]}; do
      FULL_PATH=`echo ${FILE} | awk "${AWKCMD}"`
      FILE_PATH=`dirname ${FULL_PATH}`
      FILE_NAME=`basename ${FULL_PATH}`
      mkdir -p ${SCRATCH_DIR}/${FILE_PATH}
      if [ -f ${RELEASE_DIR}/${FILE_PATH}/${FILE_NAME} ]; then
         cp ${RELEASE_DIR}/${FILE_PATH}/${FILE_NAME} ${SCRATCH_DIR}/${FILE_PATH}
      else
	 touch ${RELEASE_DIR}/${FILE_PATH}/${FILE_NAME}
      fi
   done
   PATCHLEVEL=`echo ${FILE_LIST} | awk "${AWKCM2}"`
   ${PACHPROG} -d ${SCRATCH_DIR} ${PATCHLEVEL} --backup < ${FLUX_PATCH}
   for FILE in ${FILE_LIST[@]}; do
      FULL_PATH=`echo ${FILE} | awk "${AWKCMD}"`
      FILE_PATH=`dirname ${FULL_PATH}`
      FILE_NAME=`basename ${FULL_PATH}`
      ${DIFFPROG} -u ${FULL_PATH}.orig ${FULL_PATH} --label=${FULL_PATH}.orig --label=${FULL_PATH} >> ${PATCH_FILE}
   done
}

function remove_file () {
   PATCH_SUFFIX=${1}
   HALF_PATH=${2}
   PATCH_FILE=${OUTPUT_DIR}/diff-${PATCH_SUFFIX}
   FULL_PATH=${RELEASE_DIR}/${HALF_PATH}
   ${DIFFPROG} -u ${FULL_PATH} /dev/null --label=${HALF_PATH} --label=/dev/null >> ${PATCH_FILE}
}

# MAIN ###########################################

rm -rf ${EXPANSE}/scratch
mkdir -p ${OUTPUT_DIR} ${EXPANSE}/scratch
pattern="^gdb"
main=`cd $BUGDIFF && find * -type d | sort | ${GREPPROG} -E $pattern`
reset_patch ${MAIN_SUFFIX}
produce_patch ${MAIN_SUFFIX} main[@]
regenerate_patch ${MAIN_SUFFIX} patch-gdb_Makefile.in
regenerate_patch ${MAIN_SUFFIX} patch-gdb_aarch64-linux-tdep.c
regenerate_patch ${MAIN_SUFFIX} patch-gdb_amd64-tdep.h
regenerate_patch ${MAIN_SUFFIX} patch-gdb_bsd-kvm.c
regenerate_patch ${MAIN_SUFFIX} patch-gdb_configure.host
regenerate_patch ${MAIN_SUFFIX} patch-gdb_configure.nat
regenerate_patch ${MAIN_SUFFIX} patch-gdb_configure.tgt
regenerate_patch ${MAIN_SUFFIX} patch-gdb_fbsd-nat.c
regenerate_patch ${MAIN_SUFFIX} patch-gdb_gdb.c
regenerate_patch ${MAIN_SUFFIX} patch-gdb_gdb__wchar.h
regenerate_patch ${MAIN_SUFFIX} patch-gdb_i386-bsd-nat.c
regenerate_patch ${MAIN_SUFFIX} patch-gdb_i386-fbsd-nat.c
regenerate_patch ${MAIN_SUFFIX} patch-gdb_i386-tdep.h
regenerate_patch ${MAIN_SUFFIX} patch-gdb_inflow.c
regenerate_patch ${MAIN_SUFFIX} patch-gdb_osabi.c
regenerate_patch ${MAIN_SUFFIX} patch-gdb_osabi.h
regenerate_patch ${MAIN_SUFFIX} patch-gdb_python_python-config.py
regenerate_patch ${MAIN_SUFFIX} patch-gdb_sparc64-linux-tdep.c

reset_patch ${REST_SUFFIX}
regenerate_patch ${REST_SUFFIX} patch-bfd_config.bfd
regenerate_patch ${REST_SUFFIX} patch-gdbsupport_common-defs.h
regenerate_patch ${REST_SUFFIX} patch-gnulib_import_stddef.in.h
regenerate_patch ${REST_SUFFIX} patch-include_elf_common.h

# copy to generated directory
mkdir -p "${GENERATED_DIR}"
for part in gdb rest; do
  cp -RH ${OUTPUT_DIR}/patch-diff-${part} ${GENERATED_DIR}/patch-diff-${part}
done
