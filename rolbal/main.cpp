#include "RolBal.h"
#include <blib/Util.h>
#include <blib/util/FileSystem.h>


#pragma comment(lib, "blib.lib")

int main(int argc, char* argv[])
{
	blib::util::fixConsole();
	blib::util::FileSystem::registerHandler(new blib::util::PhysicalFileSystemHandler(""));
	blib::util::FileSystem::registerHandler(new blib::util::PhysicalFileSystemHandler(".."));
	blib::util::FileSystem::registerHandler(new blib::util::PhysicalFileSystemHandler("../blib"));

	blib::App* app = new RolBal();
	app->start(true);
	delete app;



}