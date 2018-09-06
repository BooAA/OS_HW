#include <iostream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <cstring>
#include <stdlib.h>

bool select_by_inode = false;
bool select_by_filename = false;
bool select_by_min_size = false;
bool select_by_max_size = false;

void my_find(std::string dir_path, long inode_num, std::string target_file_name, double min_size, double max_size){
  DIR* d_stream;
  dirent* d_entry;
  d_stream = opendir(dir_path.c_str());
  while (d_entry = readdir(d_stream)){
    if (d_entry == NULL) break;
    if (std::strcmp(d_entry->d_name, ".") == 0 || std::strcmp(d_entry->d_name, "..") == 0) continue;
    std::string file_path;
    if (dir_path.back() == '/') file_path = dir_path + d_entry->d_name;
    else  file_path = dir_path + "/" + d_entry->d_name;
    struct stat buf;
    stat(file_path.c_str(), &buf);
    double file_size = buf.st_size/1048576.0;

    if (d_entry->d_type == DT_DIR)
      my_find(file_path, inode_num, target_file_name, min_size, max_size); 

    if (select_by_inode){ 
      if (d_entry->d_ino == inode_num){
	std::cout << file_path << " " << inode_num << " " << file_size << "MB\n";
	break;
      }
      else continue;
    }

    if (select_by_filename && target_file_name != d_entry->d_name)
      continue;


    if (select_by_min_size && file_size < min_size)
      continue;
    if (select_by_max_size && file_size > max_size)
      continue;
    std::cout << file_path << " " << d_entry->d_ino << " " << file_size << "MB\n";
  }
}

int main(int argc, char* argv[]){
  std::string dir_path = argv[1];
  long inode_num;
  std::string target_file_name;
  double min_size, max_size;

  for (int i = 2; i < argc; i += 2){
    if (std::strcmp(argv[i], "-inode") == 0){
      select_by_inode = true;
      inode_num = atol(argv[i+1]);
    }
    else if (std::strcmp(argv[i], "-name") == 0){
      select_by_filename = true;    
      target_file_name = argv[i+1];
    }
    else if (std::strcmp(argv[i], "-size_min") == 0){
      select_by_min_size = true;	
      min_size = atof(argv[i+1]);
    }
    else if (std::strcmp(argv[i], "-size_max") == 0){
      select_by_max_size = true;
      max_size = atof(argv[i+1]);
    } 
  }
  my_find(dir_path, inode_num, target_file_name, min_size, max_size);
  
  return 0;
}
