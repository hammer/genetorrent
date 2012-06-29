echo "Running aclocal --force -I m4"
aclocal --force 
echo "Running automake --foreign"
automake --foreign --add-missing
echo "Running autoconf -f"
autoconf -f
