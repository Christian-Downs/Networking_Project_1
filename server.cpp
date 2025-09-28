/*
 * CS447 P1 Server
 * ----------------------------
 *  Author: Christian Downs
 *  Date:   09/27/2025
 *  Licence: MIT Licence
 *  Description: This is the starter code for CS447 Fall 2025 P1 server.This code is based on the simple stream server code
 *      found on Beej's Guide to Network programming at https://beej.us/guide/bgnet/html/#a-simple-stream-server.
 *      The code was adapted to use C++20 features like std::jthread for concurrency.
 *
 *      This code can be compiled using:
 *           g++ -std=c++20 -Wall -pthread server.cpp -o server
 *
 *      Use this code as the base for your server implmentation.
 *
 */

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <system_error>
#include <map>
#include <fstream>

// C headers for socket API
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "p1_helper.h"
using namespace std;

// #define PORT "3490"
#define BACKLOG 10
#define MAXDATASIZE 1000

std::string coursesToString(std::vector<Course> courses)
{
  std::stringstream output;
  for (Course course : courses)
  {
    output << course.course_code << " " << course.title << "\n";
  }
  return output.str();
}

std::string courseToString(Course course)
{
  std::stringstream output;
  output << "Course Code: " << course.course_code << endl;
  output << "Title: " << course.title << endl;
  output << "Subject: " << course.subject << endl;
  output << "Instructor: " << course.instructor << endl;
  output << "Seat Capacity: " << course.capacity << endl;
  output << "Available Seats: " << course.seats_available << endl;
  output << "Prereqs: ";

  for (int i = 0; i < course.prerequisites.size(); i++)
  {
    string prereq = course.prerequisites[i];
    output << prereq;
    if (i != course.prerequisites.size() - 1)
    {
      output << ", ";
    }
  }
  output << endl;
  output << "Description: " << course.description << endl;
  return output.str();
}

bool isOption(string command)
{
  if (command == "HELP" || command == "CATALOG" || command == "ENROLLMENT" || command == "MYCOURSES" || command == "LIST" || command == "VIEWGRADES" || command == "BYE" || (command.find("LIST") != string::npos) || (command.find("SEARCH") != string::npos) || (command.find("SHOW") != string::npos) || (command.find("ENROLL") != string::npos) || (command.find("DROP") != string::npos))
  {
    return true;
  }

  return false;
}

// Helper function to get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void send_back(int pid, string message)
{
  std::string msg_str = message + "\n";
  const char *msg = msg_str.c_str();
  if (send(pid, msg, strlen(msg), 0) == -1)
  {
    perror("send");
  }
}

int message_handler(int pid, string message, string &mode, vector<Course> &courses, vector<string> &enrollmentHistory)
{
  try
  {
    if (!isOption(message))
    {
      send_back(pid, "400 Command not avaliable!");
      return 1;
    }
    else if (message == "BYE")
    {
      send_back(pid, "200 BYE");
      return 0;
    }
    else if (message == "HELP")
    {
      stringstream output;
      output << "200 Possible Commands: " << endl;
      if (mode == "ENROLLMENT")
      {
        output << "\tENROLL <course_code> - This command enrolls a client in a course. The server replies with 250 on success, 403 if the course is full, or 404 if the course is not found. Prerequisites for a course are considered met if prerequisite course(s) are listed in the current enrollment history." << endl;
        output << "\tDROP <course_code> - This command allows a client to drop a course. The server replies with 250 on success or 404 if the course was not enrolled by the client. Dropping a course removes it from the student\’s active enrollment." << endl;
      }
      else if (mode == "CATALOG")
      {
        output << "\tLIST [filter] - The LIST command lists all available courses, optionally filtered by subject, instructor, or course - code.The server replies with 250 and the list of courses, or 304 if no courses are available." << endl;
        output << "\tSEARCH <filter> <search-term> - The SEARCH command searches for courses by a specified <filter> (subject, instructor, or course-code) and <search-term>. The server replies with 250 and a list of matching courses, or 304 if none are found." << endl;
        output << "\tSHOW <course_code> [availability] - The SHOW command displays details for a specific course. When the optional [availability] argument is included, the server should only list the course\’s availability status and the number of available seats. Without the optional argument, the server should provide the full course description. The server replies with 250 and the requested details, or 404 if the course is not found." << endl;
      }
      else if (mode == "MYCOURSES")
      {
        output << "\tLIST - This command displays the student\’s current enrollment (and by that virtue the history). The server replies with 250 and the list of courses, or 304 if no courses are found." << endl;
        output << "\tVIEWGRADES - This command shows the student\’s grades for completed courses. The server replies with 250 or 304 if no grades are available." << endl;
      }
      else
      {
        output << "\tCATALOG - This command enables clients to access the course catalog. Success is acknowledged by server reply code is 210. " << endl;
        output << "\tENROLLMENT - This command allows clients to enroll in or drop courses. The server\’s reply code is 220. " << endl;
        output << "\tMYCOURSES - This mode provides clients with functionalities to manage their academic schedules. The correct server reply code is 230. " << endl;
        output << "\tBYE - This command closes the connection and requests a graceful exit. The server\’s reply code is 200." << endl;
        send_back(pid, output.str());
        return 1;
      }

      
      output << endl
             << "SWITCH MODE:" << endl;
      output << (mode == "CATALOG" ? "" : "\tCATALOG - This command enables clients to access the course catalog. Success is acknowledged by server reply code is 210. \n");
      output << (mode == "ENROLLMENT" ? "" : "\tENROLLMENT - This command allows clients to enroll in or drop courses. The server\’s reply code is 220. \n");
      output << (mode == "MYCOURSES" ? "" : "\tMYCOURSES - This mode provides clients with functionalities to manage their academic schedules. The correct server reply code is 230. \n");
      output
          << endl
          << "\tBYE - This command closes the connection and requests a graceful exit. The server\’s reply code is 200." << endl;

      send_back(pid, output.str());
      return 1;
    }
    else if (message == "CATALOG")
    {
      mode = "CATALOG";
      send_back(pid, "210 Switched to CATALOG Mode");
      return 1;
    }
    else if (message == "ENROLLMENT")
    {
      mode = "ENROLLMENT";
      send_back(pid, "220 Switched to ENROLLMENT Mode");
      return 1;
    }
    else if (message == "MYCOURSES")
    {
      mode = "MYCOURSES";
      send_back(pid, "230 Switched to MYCOURSES Mode");
      return 1;
    }
    else if (mode == "NO MODE")
    {
      send_back(pid, "503 Bad sequence of commands. Must enter a mode first.");
      return 1;
    }
    else if ((message.find("SEARCH") != string::npos))
    {
      if (mode == "CATALOG")
      {
        string filter, search_term;
        message.erase(0, message.find(" ") + 1);
        filter = message.substr(0, message.find(" "));
        message.erase(0, message.find(" ") + 1);
        search_term = message.substr(0, message.find(" "));
        if (filter == "" || search_term == "")
        {
          send_back(pid, "400 NEED FILTER AND SEARCH TERM");
          return 1;
        }

        vector<Course> returnedCourses = search_courses(courses, filter, search_term);
        if (returnedCourses.size() == 0)
        {
          send_back(pid, "304 No classes found!");
          return 1;
        }
        std::stringstream output;
        output << "250" << "\n";

        for (Course course : returnedCourses)
        {
          output << course.course_code << " " << course.title << "\n";
        }

        send_back(pid, output.str());
        return 1;
      }
      else
      {
        send_back(pid, "400 Need to switch to the CATALOG MODE!");
        return 1;
      }
    }
    else if (message.find("LIST") != string::npos)
    {
      if (mode == "CATALOG")
      {
        string filter = "ALL";
        string search_term = "";
        if (message.find(" ") != string::npos)
        {
          message.erase(0, message.find(" ") + 1);
          filter = message.substr(0, message.find(" "));
          message.erase(0, message.find(" ") + 1);
          search_term = message.substr(0, message.find(" "));
        }
        vector<Course> courseList = search_courses(courses, filter, search_term);

        if (courseList.size() == 0)
        {
          send_back(pid, "304 No classes found!");
          return 1;
        }

        string courseString = "250 \n" + coursesToString(courseList);

        send_back(pid, courseString);
        return 1;
      }
      else if (mode == "MYCOURSES")
      {
        if (enrollmentHistory.size() == 0)
        {
          send_back(pid, "304 NO CONTENT you haven't enrolled in any classes!");
          return 1;
        }
        stringstream output;
        output << "250 Enrollment Histroy:" << endl;
        for (string course : enrollmentHistory)
        {
          output << "\t" << course << endl;
        }
        send_back(pid, output.str());
        return 1;
      }
      else
      {
        send_back(pid, "400 Need to switch to the CATALOG OR MYCOURSES MODE!");
        return 1;
      }
    }
    else if (message.find("SHOW") != string::npos)
    {
      if (mode == "CATALOG")
      {
        message.erase(0, message.find(" ") + 1);
        string course_code, availability = "";
        if (message.find(" ") != string::npos)
        {
          course_code = message.substr(0, message.find(" "));
          message.erase(0, message.find(" ") + 1);
          availability = message;
        }
        else
        {
          course_code = message;
        }

        Course course = get_course_by_code(courses, course_code);

        if (course.title == "")
        {
          send_back(pid, "304 No Class Found!");
          return 1;
        }

        stringstream output;
        output << "250";
        if (availability == "availability")
        {
          output << " Availability " << (course.seats_available > 0 ? "Open, " : "Close, ") << "Seats: " << course.seats_available;
        }
        else
        {
          output << endl
                 << courseToString(course);
        }
        send_back(pid, output.str());
        return 1;
      }
      else
      {
        send_back(pid, "400 Need to switch to the CATALOG MODE!");
        return 1;
      }
    }
    else if (message.find("ENROLL") != string::npos)
    {
      if (mode != "ENROLLMENT")
      {
        send_back(pid, "400 Need to switch to the ENROLLMENT MODE!");
        return 1;
      }
      string course_code;
      message.erase(0, message.find(" ") + 1);
      course_code = message;
      Course course = get_course_by_code(courses, course_code);
      if (course.title == "NOT FOUND")
      {
        send_back(pid, "404 NOT FOUND. Course Not Found.");
        return 1;
      }
      for (string prereq : course.prerequisites)
      {
        bool found = false;
        for (string taken : enrollmentHistory)
        {
          if (prereq == taken)
          {
            found = true;
            break;
          }
        }
        if (!found)
        {
          send_back(pid, "403 FORBIDDEN. Prerequisites not met.");
          return 1;
        }
      }

      enrollmentHistory.push_back(course_code);

      send_back(pid, "250 ENROLLMENT SUCCESSFUL.");
      return 1;
    }
    else if (message.find("DROP") != string::npos)
    {
      if (mode != "ENROLLMENT")
      {
        send_back(pid, "400 Need to switch to the ENROLLMENT MODE!");
        return 1;
      }
      message.erase(0, message.find(" ") + 1);

      for (int i = 0; i < enrollmentHistory.size(); i++)
      {
        if (message == enrollmentHistory[i])
        {
          enrollmentHistory.erase(enrollmentHistory.begin() + i);
          send_back(pid, "250 Dropped course.");
          return 1;
        }
      }
      send_back(pid, "404 NOT FOUND. Class not found in enrollment history.");
      return 1;
    }
    else if (message == "VIEWGRADES")
    {
      send_back(pid, "304 NO CONTENT. No grades found.");
      return 1;
    }
    else
    {
      send_back(pid, "400 BAD REQUEST");
      return 1;
    }
    send_back(pid, "400 BAD REQUEST");
    return 1;
  }
  catch (...)
  {
    send_back(pid, "500 INTERNAL SERVER ERROR");
    return 1;
  }
}

// Function to handle a single client connection in its own thread
void handle_client(int pid, struct sockaddr_storage their_addr)
{
  // A temporary buffer for the client's IP address string
  char s[INET6_ADDRSTRLEN];
  inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
  std::cout << "server: got connection from " << s << std::endl;
  int numbytes;
  bool initalized = false;

  char buf[MAXDATASIZE];
  string mode = "NO MODE";

  vector<Course> courses = load_courses_from_db("courses.db");
  vector<string> enrollmentHistory = {};

  while (true)
  {
    numbytes = recv(pid, buf, MAXDATASIZE - 1, 0);
    if (numbytes == 0)
    {
      // client closed
      break;
    }
    if (numbytes < 0)
    {
      perror("recv");
      break;
    }
    buf[numbytes] = '\0';

    buf[strcspn(buf, "\r\n")] = '\0'; // Strip newline characters
    string message_string = buf;
    printf("server: received '%s'\n", buf);
    if (!initalized)
    {
      if (message_string.find("IAM") != string::npos)
      {
        initalized = true;
        string name = message_string.substr(message_string.find(" ") + 1);
        string first_name;
        if (name.find(" ") != string::npos)
        {
          first_name = name;
        }
        else
        {
          first_name = name.substr(0, name.find(" "));
        }
        send_back(pid, "200 Welcome " + first_name + "@" + s);
        continue;
      }
      else
      {

        if (isOption(buf))
        {
          send_back(pid, "403 Bad sequence of commands. Must sign in first!");
          continue;
        }
        else
        {
          send_back(pid, "400 Please Sign-In first!");
          continue;
        }
      }
    }
    if (message_handler(pid, message_string, mode, courses, enrollmentHistory) == 0)
    {
      break;
    }
  }
  // optional echo back
  // if (send(new_fd, buf, numbytes, 0) == -1) perror("send");

  // Close the socket for this connection
  close(pid);
  std::cout << "server: connection with " << s << " closed." << std::endl;
}

map<string, string> read_config_file(string fileName)
{
  map<string, string> configMap;
  ifstream file(fileName);
  string str;
  while (getline(file, str))
  {
    string key = str.substr(0, str.find('='));
    string value = str.substr(str.find('=') + 1);
    configMap[key] = value;
  }
  return configMap;
}

int main(int argumentCount, char *argumentArray[])
{
  int sockfd, new_fd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  int yes = 1;
  int rv;

  std::map<string, string> configMap = read_config_file(argumentArray[1]);

  const char *PORT = configMap["PORT"].c_str();

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("server: socket");
      continue;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("server: bind");
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo);

  if (p == NULL)
  {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }
  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("listen");
    exit(1);
  }
  std::cout << "server: waiting for connections..." << std::endl;

  while (true)
  {
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd == -1)
    {
      perror("accept");
      continue;
    }

    // Create a new thread to handle the accepted connection
    // std::jthread automatically joins upon destruction
    std::jthread(handle_client, new_fd, their_addr).detach();
  }

  // The main loop will never exit, so this is unreachable.
  close(sockfd);
  return 0;
}
