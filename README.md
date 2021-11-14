![ToolchainGenericDS](img/TGDS-Logo.png)

NTR/TWL SDK: TGDS1.65

master: Development branch. Use TGDS1.65: branch for stable features.

This is the ToolchainGenericDS-UnitTest project:

1.	Compile Toolchain:
To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds :
Then simply extract the project somewhere.

2.	Compile this project: 
Open msys, through msys commands head to the directory your extracted this project.
Then write:
make clean <enter>
make <enter>

After compiling, run the example in NDS. 

Project Specific description:
	Press Y button to perform a suite of tests to test Clang, and C/C++ functinality. Performs C/C++ tests, and should pass all of them.

/release folder has the latest binary precompiled for your convenience.

Latest stable release: https://bitbucket.org/Coto88/ToolchainGenericDS-UnitTest/get/TGDS1.65.zip

	
Coto

Test Unit results in TGDS:

Malloc:

![ToolchainGenericDS](img/mallocComparison.png)


TGDS Filesystem:

![ToolchainGenericDS](img/fstest1.png)

![ToolchainGenericDS](img/fstest2.png)

![ToolchainGenericDS](img/fstest3.png)

![ToolchainGenericDS](img/fstest4.png)
