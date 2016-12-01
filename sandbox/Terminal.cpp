#include "Headers.hpp"

#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>

#if __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif

#define FAIL_FATAL(X) if((X) == -1) { printf("Error: (%d), %s\n",errno,strerror(errno)); exit(errno); }

int main(int argc, char** argv) {
  int masterfd;
  char* slaveFileName = new char[4096];
  termios* slaveTerminalParameters = new termios();
  winsize* windowParameters = new winsize();

  pid_t pid = forkpty(
    &masterfd,
    slaveFileName,
    slaveTerminalParameters,
    windowParameters);
  switch (pid) {
  case -1:
    FAIL_FATAL(pid);
  case 0:
    // child
    cout << "Child process" << endl;
    execlp("bash", "/bin/bash", NULL);
    exit(0);
    break;
  default:
    // parent
    cout << "pty opened " << masterfd << endl;
    cout << slaveFileName << endl;
    cout << windowParameters->ws_row << " "
         << windowParameters->ws_col << " "
         << windowParameters->ws_xpixel << " "
         << windowParameters->ws_ypixel << endl;
    // Whether the TE should keep running.
    bool run = true;

    // remove the echo
    struct termios tios;
    tcgetattr(masterfd, &tios);
    tios.c_lflag &= ~(ECHO | ECHONL);
    tcsetattr(masterfd, TCSAFLUSH, &tios);

    // TE sends/receives data to/from the shell one char at a time.
    char b, c;

    while (run)
    {
      // Data structures needed for select() and
      // non-blocking I/O.
      fd_set rfd;
      fd_set wfd;
      fd_set efd;
      timeval tv;

      FD_ZERO(&rfd);
      FD_ZERO(&wfd);
      FD_ZERO(&efd);
      FD_SET(masterfd, &rfd);
      FD_SET(STDIN_FILENO, &rfd);
      tv.tv_sec = 0;
      tv.tv_usec = 100000;
      select(masterfd + 1, &rfd, &wfd, &efd, &tv);

      // Check for data to receive; the received
      // data includes also the data previously sent
      // on the same master descriptor (line 90).
      if (FD_ISSET(masterfd, &rfd))
      {
        int rc = read(masterfd, &c, 1);
        FAIL_FATAL(rc);
        if (rc > 0)
          write(STDOUT_FILENO, &c, 1);
        else if (rc==0)
          run = false;
        else
          cout << "This shouldn't happen\n";
      }

      // Check for data to send.
      if (FD_ISSET(STDIN_FILENO, &rfd))
      {
        read(STDIN_FILENO, &b, 1);
        write(masterfd, &b, 1);
      }
    }
    break;
  }

  return 0;
}
