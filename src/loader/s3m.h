#ifndef _LOADER_S3M_
#define _LOADER_S3M_

#include <fstream>
#include <memory>

struct Module;
extern std::shared_ptr<Module> load_s3m(std::ifstream& s3m);

#endif