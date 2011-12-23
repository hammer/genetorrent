echo "Running autoheader -f"
autoheader -f
echo "Running aclocal --force -I m4"
aclocal --force -I m4
echo "Running automake --foreign"
automake --foreign
echo "Running autoconf -f"
autoconf -f
