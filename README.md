# salsa-repo
source codes of Salsa cipher

## Running

Clone the github repo first.
then build and run the PNB search program:

```sh
g++ -std=c++2c -O3 -flto altaumstylepnb.cpp -o output
./output <neutrality_measure> [log] [segments]
```

`log` enables logging to a file so you can see the output (accepted values: `log`, `LOG`, or `1`).

`segments` prints per-keyword segment summaries for both PNBs and non-PNBs (accepted values: `seg`, `segment`, or `segments`).

Example:

```sh
g++ -std=c++20 -O3 altaumstylepnb.cpp
./a.out <neutrality_measure> log segments
```
