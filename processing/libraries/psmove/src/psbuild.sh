#cp -R ../../../../build/io ./io

cp ../../../../build/libpsmove_java.jnilib ../library/libpsmove_java.jnilib
cp library.properties build
cp export.txt build
cp psmove.java build

cd build

javac -classpath "/Applications/Processing.app/Contents/Resources/Java/core.jar:../../../../../build/psmoveapi.jar" -d . psmove.java
rm psmove.java
zip -r ../../library/psmove.jar .

cd ..
