/*
  Complete test suite for most system calls implemented in lab 1.

   This program tests the minimum file limit and verifies all
   returned data.

   Tested requirements:
     - write must return number of bytes written
     - create must return whether the file was created
     - a user program must be able to have 128 files open
       at the same time
     - opening a file must return a valid file descriptor
     - opening a file mulitiple times must return unique
       file descriptors
     - reading from a closed file must fail
     - reading from an invalid file descriptor must fail
     - attempting to open missing files must fail
     - read must return number of bytes read
     - read after write must return written data
     - reading from console be implemented
     - exit must terminate the user program

   Untested requirements:
     - halt must shut down the system
     - file descriptors must be closed on process exit
*/
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <stdarg.h>

#define FD_TEST_COUNT 128
#define READ_SIZE 50
#define READ_CONSOLE_COUNT 10

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define TITLE(x); printf(WHT x RESET)
#define ERROR(x, ...) printf(RED "ERR: " x RESET, ##__VA_ARGS__); halt()
#define SUCCESS(x) printf(GRN x RESET)

int main(void)
{
  char *dummyprint = "Hello, world!\n";
  char *testdata = "sample file content";
  bool created;
  bool removed;
  int fd;
  int bytes_written;
  int bytes_read;
  int size;
  char sbuf[READ_SIZE];



  TITLE("TEST 1: Printing text\n");
  bytes_written = write(STDOUT_FILENO, dummyprint, strlen(dummyprint));
  if (bytes_written < 0 || (size_t)bytes_written != strlen(dummyprint))
  {
    ERROR("Incorrect number of written bytes returned from SYS_WRITE.\n");
  }
  else
  {
    SUCCESS("TEST 1: Passed\n");
  }



  TITLE("TEST 2: Creating file\n");
  created = create("test0", strlen(testdata));
  if (!created)
  {
    ERROR("Could not create file \"test0\", does it already exist?\n");
  }

  created = create("test0", strlen(testdata));
  if (created)
  {
    ERROR("Succeeded in creating already existing file.\n");
  }

  SUCCESS("TEST 2: Passed\n");



  TITLE("TEST 3: Opening files\n");
  int file_descriptors[FD_TEST_COUNT];
  int i;

  printf("Opening %d files", FD_TEST_COUNT);
  for (i = 0; i < FD_TEST_COUNT; ++i)
  {
    fd = open("test0");
    if (fd == -1)
    {
      printf("\n");
      ERROR("Failed to open file, iteration %d.\n", i + 1);
    }

    if (fd == STDIN_FILENO || fd == STDOUT_FILENO)
    {
      printf("\n");
      ERROR("Opened file with invalid file descriptor.\n");
    }

    int j;
    for (j = 0; j < i; ++j)
    {
      if (file_descriptors[j] == fd)
      {
	printf("\n");
        ERROR("Opened file with reoccuring file descriptor.\n");
      }
    }

    file_descriptors[i] = fd;
    printf(".");
  }

  printf("\nDone!\n");
  printf("Closing files");

  for (i = 0; i < FD_TEST_COUNT; ++i)
  {
    close(file_descriptors[i]);
    bytes_read = read(file_descriptors[i], sbuf, READ_SIZE);
    if (bytes_read != -1)
    {
      printf("\n");
      ERROR("Successfully read from closed file.\n");
    }

    printf(".");
  }

  printf("\nDone!\n");

  bytes_read = read(STDOUT_FILENO, sbuf, READ_SIZE);
  if (bytes_read != -1)
  {
    ERROR("Successfully read from missing file descriptor.\n");
  }

  fd = open("foobar");
  if (fd != -1)
  {
    ERROR("Successfully opened missing file.\n");
  }

  SUCCESS("TEST 3: Passed\n");



  TITLE("TEST 4: Writing to file\n");
  fd = open("test0");
  bytes_written = write(fd, testdata, strlen(testdata));
  if (bytes_written < 0 || (size_t)bytes_written != strlen(testdata))
  {
    ERROR("Failed to write %d bytes to file, wrote %d.\n", strlen(testdata), bytes_written);
  }
  close(fd);

  SUCCESS("TEST 4: Passed\n");



  TITLE("TEST 5: Reading from file\n");
  fd = open("test0");
  bytes_read = read(fd, sbuf, READ_SIZE);
  if (bytes_read < 0 || (size_t)bytes_read != strlen(testdata))
  {
    ERROR("Failed to read %d bytes from file, read %d.\n", strlen(testdata), bytes_read);
  }

  if (memcmp(sbuf, testdata, strlen(testdata)) != 0)
  {
    ERROR("Read content does not match what was written to file.\n");
  }
  close(fd);

  SUCCESS("TEST 5: Passed\n");



  TITLE("TEST 6: Reading from console\n");
  printf("Type 10 characters: ");
  bytes_read = read(STDIN_FILENO, sbuf, READ_CONSOLE_COUNT);
  printf("\n");
  if (bytes_read != READ_CONSOLE_COUNT)
  {
    ERROR("Failed to read %d characters from console, read %d.\n", READ_CONSOLE_COUNT, bytes_read);
  }

  SUCCESS("TEST 6: Passed\n");

  // TITLE("TEST 7: ")

  TITLE("TEST 7: Size of File\n");
  created = create("test1", 10);
  fd = open("test1");
  size = filesize(fd);
  if(size != 10)
  {
    ERROR("File of Size: %d, but reported by syscall as %d",10,size); 
  }
  close(fd);

  SUCCESS("TEST 7: Passed\n");

  TITLE("TEST 8: Testing REMOVE\n");

  created = create("test2", 20);
  removed = remove("test2");

  if(removed)
  {
    fd = open("test2");
    if(fd > 0)
    {
      ERROR("Opened a Deleted File!\n");
    }
    else
    {
      SUCCESS("TEST 8: Passed\n");
    }
  }
  else
  {
    ERROR("Cannot remove the file, \"test2\", already deleted?\n");
  }

  TITLE("TEST 9: Testing SEEK\n");

  created = create("test3",20);
  fd = open("test3");
  bytes_written = write(fd, testdata, strlen(testdata));  
  seek(fd, 5);
  bytes_read = read(fd, sbuf, READ_SIZE);
  if (bytes_read < 0 || (size_t)bytes_read != strlen(testdata)-4)
  {
    ERROR("Failed to read %d bytes from file, read %d.\n", strlen(testdata), bytes_read);
  }
  else
  {
    SUCCESS("After doing a seek for 5 positions, number of bytes read strlen(testdata) - (5-1)\n");
  }
  

  // if (memcmp(sbuf, testdata, strlen(testdata)) != 0)
  // {
  //   ERROR("Read content does not match what was written to file.\n");
  // }


  TITLE("TEST 10: Testing HALT\n") 


  TITLE("The test suite would now test SYS_HALT, if you are able to see another print, statement.....\n");

  // TITLE("The test suite should now exit. Since SYS_WAIT is not implemented yet, the program should hang. ");
  // TITLE("If it does, it means that all tests were successful.\n");
//  exit(0);

//  ERROR("ERR: Thread did not exit.\n");
  halt();
  ERROR("Why are you seeing this print?");
}
