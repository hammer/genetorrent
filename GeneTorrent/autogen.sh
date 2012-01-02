echo "Running aclocal --force -I m4"
aclocal --force -I m4
echo "Running autoheader -f"
autoheader -f
echo "Running autoconf -f"
autoconf -f
echo "Running automake --foreign"
automake --foreign
