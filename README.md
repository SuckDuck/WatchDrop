# WatchDrop
`watchdrop` is a small simple program for continuously look for certain files inside a folder, and move/remove them once
they appear. It doesn't provide any functionality beyond that and it's probably not so well made since it is just a tool that
i needed as part of my workflow and my C learning process.

The project relies on [`Native File Dialog Extended`](https://github.com/btzy/nativefiledialog-extended) for the folder selection
pop up when moving a file and since it depends on `GTK3` for work and `cmake` for build, make sure to have both installed in your
system before continue with this installation.

## Build and Install

````
git clone https://github.com/SuckDuck/WatchDrop.git
cd watchdrop
git clone https://github.com/btzy/nativefiledialog-extended.git
cd nativefiledialog-extended
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd ../../
sudo make install
````

## How to use
For using `watchdrop` you should first pass it as params the files it should look for, as `-me .sh` *the flags available are below*. Then you should specify what route it should look for files, it can be a relative path, but you probably should use
an absolute one. Then optionally you can specify a command with it's own options that `watchdrop` should execute at start up, if
you specify one then `watchdrop` is going to live until it's child process has ended.

* **-me \<pattern>**: It looks for files that include the specified value in their name and asks the user through a file dialog pop-up where to save this file. It is intended to work with extensions `.png, .zip, .pdf...` but could have other uses too
* **-de \<pattern>**: As `-me`, it searches for names partially, but deletes matches
* **-mf \<filename>**: Same behavior as `-me` but works *only* for files whose name is equal to the specified value
* **-df \<filename>**: Same behavior as `-de` but works only with names equal to those specified with `-mf`
* **-ig \<filename>**: It ignores a file, even if it is affected by other flags. It is intended to exclude special files from the other flags

## Examples
This command should continuously monitor an user's Downloads folder looking for songs, so once the user downloads one, a pop-up asking where to save it jumps up for the user, working similarly to when you download an image from your browser, but for songs
```
watchdrop -me .mp3 /home/user/Downloads
```

Let's say you made a script that after some process generates some data, so once it's done that data should be saved where the user says, but you don't want to implement the file dialog yourself, so you just make the script place the output data in a generic folder and run the following
```
watchdrop -me .data /your/script/output /your/script.py some args
```