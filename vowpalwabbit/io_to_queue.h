#pragma once
 
#include <thread>
#include <future>
#include <mutex> 
#include <chrono>  
 
//Adds all lines of input to the input queue. The function version on this branch (multithread_parser_with_passes) supports passes.
inline void io_lines_toqueue(vw& all){

  std::mutex mut;
  std::unique_lock<std::mutex> lock(mut);
   
  while(!all.example_parser->done) {

    char* line = nullptr;
    bool should_finish = false;

    while(!should_finish){
      should_finish = all.example_parser->input_file_reader(all, line);
    }

    while(!all.example_parser->done_with_io) {
      all.example_parser->can_end_pass.wait(lock);
    }
    all.example_parser->done_with_io.store(false);

  }

}
 

inline bool read_input_file_ascii(vw& all, char *&line) {
 
 size_t num_chars_initial = all.example_parser->input->readto(line, '\n');
 
 bool finish = false;
 
  std::vector<char> *byte_array = new std::vector<char>();
  byte_array->resize(num_chars_initial); // Note: This byte_array is NOT null terminated!

  memcpy(byte_array->data(), line, num_chars_initial);

  all.example_parser->io_lines.push(std::move(byte_array));
 
  if(num_chars_initial <= 0){
    finish = true;
  }
 
 return finish;
 
}
 
inline bool read_input_file_binary(vw& all, char *&line) {

  size_t num_chars_initial;
  io_buf* input = all.example_parser->input.get();

  // Return true if no examples found in file.
  if((*input).buf_read(line, sizeof(num_chars_initial)) < sizeof(num_chars_initial)) {
    // The end of io_lines should contain a null pointer.
    std::vector<char> *byte_array = new std::vector<char>();
    all.example_parser->io_lines.push(std::move(byte_array));
    return true;
  }
  // read the example size.
  num_chars_initial = *(size_t*)line;
  line += sizeof(num_chars_initial);
  all.example_parser->input->set(line);
  (*input).buf_read(line, num_chars_initial);
 
 bool finish = false;
 
  std::vector<char> *byte_array = new std::vector<char>();
  byte_array->resize(num_chars_initial); // Note: This byte_array is NOT null terminated!

  memcpy(byte_array->data(), line, num_chars_initial);
  line += num_chars_initial;
  all.example_parser->input->set(line);


  all.example_parser->io_lines.push(std::move(byte_array));
 
 return finish;

}
 
//try to encapsulate start io thread and end io thread, so it can be moved out of parse dispatch loop.
namespace VW {
 
inline void start_io_thread(vw& all){
 
 if(!all.early_terminate) {
   all.io_thread = std::thread([&all]()
   {
     io_lines_toqueue(all);
   });
 }
 
}
 
inline void end_io_thread(vw& all){
 
 all.io_thread.join();
 
}
 
}