
echo ""
echo "================================================================="
echo "                    Building charge_profile_factory"
echo "================================================================="

is_windows(){
    unameOut="$(uname -s)"  # Linux*) MINGW*)  Darwin*)  CYGWIN*)

    case "${unameOut}" in
        CYGWIN*)    return 1;;
        MINGW*)     return 1;;
        *)          return 0
    esac
}

is_windows
if [ $? -eq 1 ]; then
    python_ext="python"
else
    python_ext="python3"
fi

#----------------------------

rm -rf ./build
eval "$python_ext setup.py build"
mv ./build/lib.*/charge_profile_factory* ./
rm -rf ./build
cd ..

echo ""

