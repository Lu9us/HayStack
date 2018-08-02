rm -r output
mkdir output
gcc main.c -D_FILE_OFFSET_BITS=64 -O3 -o output/test

gcc main.c -D_FILE_OFFSET_BITS=64 -g
cd output
cd ..
cp -rf data output
cd output
ls
time ./test --haystack "data/warmup-haystack.txt" --needles "data/warmup-needles.txt"  
time ./test --haystack "data/haystack.txt" --needles "data/needles-01.txt" 
time ./test --haystack "data/haystack.txt" --needles "data/needles-02.txt" 
#./run-all-tests -v
