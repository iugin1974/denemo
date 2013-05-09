echo extracting scheme scripts
cd utils
export PATH=$PATH:`pwd`
echo $PATH
cd ../actions/commandscripts

export DENEMO_COMMANDSCRIPTS_DIR=`pwd`
echo extracting to $DENEMO_COMMANDSCRIPTS_DIR

cd ../menus && find $1 -name "*" -exec extract_scheme '{}' \;

cd ../../

echo "#Automatically generated by extract_all.sh" >po/SCHEME_POTFILES.in
echo "actions/denemo.scm" >> po/SCHEME_POTFILES.in
ls -A1 actions/denemo-modules/*.scm  >> po/SCHEME_POTFILES.in
ls -A1 actions/commandscripts/*.scm >> po/SCHEME_POTFILES.in
echo ";automatically generated by extract_all.sh" > actions/commandscripts/init.scm
find actions/menus -name "init.scm" -exec cat '{}' \; >> actions/commandscripts/init.scm
echo "actions/commandscripts/init.scm" >>  po/SCHEME_POTFILES.in

ls -A1 src/*.[ch] > po/POTFILES.in.in
find actions/menus -name "*\.xml" -type f -exec echo "{}" \; >> po/POTFILES.in.in
cd po
cat POTFILES.in.in SCHEME_POTFILES.in > POTFILES.in
