#ifndef _LOADER_IT_
#define _LOADER_IT_

#include <fstream>
#include <memory>

struct Module;
extern std::shared_ptr<Module> load_it(std::ifstream& fs);

#endif