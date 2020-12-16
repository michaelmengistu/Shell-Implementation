#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h> 
#include <vector>
#include <string>
#include <cstring>
using namespace std;

vector<int> pipeIndex;//keeps track of when to pipe
vector<int> leftIndex; //keeps track of "<"
vector<int> rightIndex; //keeps track of "<"

/*removes leading and trailing spaces*/
void trim(string& inputString);

/*turns vector array to char array*/
char** vec_to_char_array (vector<string>& parts);

/*Tonkenizies string by a character*/ 
vector<string> split (string line, char separator);

/*separtes command line arguments*/
vector<string> paraphraser(string line);



//excute commamds
void execute(string command){
    vector<string> singleCommand = split(command, ' '); // slipt by space
    char **args = vec_to_char_array(singleCommand); //change to char array
    if(strcmp(args[0], "cd") == 0){ //handle cd 
        if(strcmp(args[1], "-") == 0){ //show current directory 
            long size;
            char *buf;
            char *ptr;
            size = pathconf(".", _PC_PATH_MAX);
            if ((buf = (char *)malloc((size_t)size)) != NULL)
                ptr = getcwd(buf, (size_t)size);
            cout << ptr <<endl;
        }
        else{
            chdir(args[1]);
        }
    }
    else{
        execvp(args[0], args);
    }
}
void execute(vector<string> command){
    string newCommand = command.at(0);
    for (int i = 1; i < command.size(); i++)
    {
        newCommand.append(" ");
        newCommand.append(command.at(i));
    }
    
    vector<string> singleCommand = split(newCommand, ' '); // slipt by space
    char **args = vec_to_char_array(singleCommand); //change to char array
    if(strcmp(args[0], "cd") == 0){ //handle cd 
        if(strcmp(args[1], "-") == 0){ //show current directory 
            long size;
            char *buf;
            char *ptr;
            size = pathconf(".", _PC_PATH_MAX);
            if ((buf = (char *)malloc((size_t)size)) != NULL)
                ptr = getcwd(buf, (size_t)size);
            cout << ptr <<endl;
        }
        else{
            chdir(args[1]);
        }
    }
    else{
        execvp(args[0], args);
    }
}




int main (){
    vector<int> bgP; // stores background processes id
    bool backgroundPrc = false;
    int start0 = dup(0); //makes copy of stdin
    int start1 = dup(1); //makes copy of stdout
    //printf("start0 = %d, start1 = %d \n", start0,start1);
    
    
    while (true){ //keeps runing shell program

        /*retores stdin and stdout*/
        dup2(start0,0);
        dup2(start1,1);

        /*checks on background processses every command*/
        for(int i=0; i < bgP.size(); i++){
            if( waitpid(bgP[i], 0, WNOHANG) == bgP[i] ){
                bgP.erase(bgP.begin() + i);
                i--;
            }
        }

        cout << "My Shell$ ";
        string inputline;
        getline (cin, inputline);   // get a line from standard input

        /*check to end shell program*/
        if (inputline == string("exit")){
            cout << "Bye!! End of shell" << endl;
            break;
        }

        trim(inputline);
        if(inputline[inputline.size()-1] == '&'){ //checks for background process
            //cout << "Background process found" << endl;
            backgroundPrc = true;
            inputline = inputline.substr(0, inputline.size()-1);
        }

        vector<string> processes = paraphraser(inputline); //splits command line into processes

        //read from processes
        /*for(int i =0 ; i < processes.size(); ++i){
            cout << processes.at(i) <<", ";
        }
        cout << endl<<endl;*/



        //one command
        if(pipeIndex.size() == 0 && leftIndex.size() == 0 && rightIndex.size() == 0){
            //cout << "ONE COMMAND.." <<endl;
            int pid = fork ();
            if (pid == 0){
                execute(processes);
            }
            else
            {
                if(!backgroundPrc){
                    wait (0); // wait for the child process 
                }
                else{
                    bgP.push_back(pid);
                }
            }
        }

        else if(pipeIndex.size() != 0){ //PIPING
            pipeIndex.pop_back();
            //cout << "PIPING..." <<endl;
            int fds[2];
            if(pipe(fds) != 0){ //check for pipe
                perror("pipe error");
            }
            int pid = fork();
            if (pid == 0)
            { // child process

                dup2(fds[1], 1);
                close(fds[0]);
                //printf("child: fds[1] = %d\n", fds[1]);

                //execute command
                execute(processes[0]);
            }
            else
            { //parent process
                if(!backgroundPrc){
                    wait(0);
                    dup2(fds[0], 0);
                    close(fds[1]);
                    //printf("parent: fds[1] = %d\n", fds[1]);

                    //exute command
                    execute(processes[2]);
                }
                else{
                    bgP.push_back(pid);
                }
            }
        }
        else if( leftIndex.size() != 0 || rightIndex.size() != 0 ){ //redirection
            //cout << "redirection.." <<endl;
            int numOfredirection = (int) leftIndex.size() + (int) rightIndex.size();
            int i=0;
            int j=0;
            int left = 0;
            int right = 0;
            //while(numOfredirection){
                //cout << "while loop:" << numOfredirection <<endl <<endl <<endl << endl;
                //cout << "i= " << i << " leftIndex.size()=" <<leftIndex.size() <<endl;
                if( i < leftIndex.size()){
                left = leftIndex.at(i);
                ++i;
                }
                else{
                    left = right +1;
                }

                //cout << "j= " << j << " rightIndex.size()=" <<rightIndex.size() <<endl;
                if( j < rightIndex.size()){
                    right = rightIndex.at(j);
                    ++j;
                }
                else{
                    right = left +1;
                }
                string filename;
                string command;
                //cout << "RIGHT= " << right <<endl;
                //cout << "LEFT= "<<left <<endl;
                if(left < right){ // do  "<" first
                    //cout << "chose LEFT" <<endl;
                    //cout << "processes..."<< processes.at(left -1) <<", " << processes.at(left +1) <<endl;
                    filename = processes.at(left -1); 
                    command = processes.at(left +1);
                    numOfredirection--;
                    
                }
                else{ // do  ">" first
                    //cout << "chose RIGHT" <<endl;
                    //cout << "processes..."<< processes.at(right+1) <<", " << processes.at(right-1) <<endl;
                    filename = processes.at(right +1); 
                    command = processes.at(right -1);
                    numOfredirection--;
                }
                int pid = fork();
                if(pid==0){
                    int fd = open (filename.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    dup2 (fd, 1);
                    execute(command);
                    close(fd);
                }
                else{
                    wait(0);
                    

                }
                
            //}
        }
        else{
            perror("error: incorrect command format");
        }

        //set back to 0
        pipeIndex.resize(0);
        leftIndex.resize(0); 
        rightIndex.resize(0);
    }
}























void trim(string& inputString){
    size_t spaceIndex = inputString.find_first_not_of(" \t");
    inputString.erase(0, spaceIndex); //removes leading spaces
 
    spaceIndex = inputString.find_last_not_of(" \t");
    if (std::string::npos != spaceIndex)
       inputString.erase(spaceIndex+1); //removes trailing spaces
}










char** vec_to_char_array (vector<string>& parts){
    char** result = new char* [parts.size() + 1];
    for(int i=0; i < parts.size(); ++i){
        result[i] = (char*) parts[i].c_str(); //changes array of string into char array
    }
    result[parts.size()] = NULL;
    return result;
}











vector<string> split (string line, char separator){
	vector<string> result;
	while (line.size()){
		size_t found = line.find_first_of (separator);
		if (found != std::string::npos){ //splits string by a character
			string part = line.substr(0, found);
			result.push_back(part);
			line = line.substr (found+1);
		}
		else{
			result.push_back (line);
			break;
		}
	}
	return result;
}















vector<string> paraphraser(string line){
    vector<string> result;
    while(line.size()){
        size_t sigQ = line.find_first_of('\''); //single quote
        size_t dubQ = line.find_first_of('\"');// double quote
        size_t pipe = line.find_first_of('|');
        size_t left = line.find_first_of('<');
        size_t right = line.find_first_of('>');
        //cout << "sigQ=" << sigQ << "  dubQ= "<< dubQ << "  pipe=" << pipe << "  left= " <<left << "  right= " << right <<endl <<endl;
        if(sigQ < dubQ && sigQ < pipe && sigQ < left && sigQ < right){ //get command before and after single quote
            //cout << "found: single quote"<<endl ;
            string part1 = line.substr(0, sigQ);
            trim(part1);
			result.push_back(part1);
			line = line.substr (sigQ+1);
            if(line.size() ==0){
                perror("paraphraser error");
            }
            string part2 =line.substr(0,  line.find_first_of('\''));
            trim(part2);
            result.push_back(part2);
			line = line.substr (line.find_first_of('\'')+1);
            //result.at(result.size()-1).insert(0,"\'");
            //result.at(result.size()-1).append("\'");
        }
        else if(dubQ < sigQ && dubQ < pipe && dubQ < left && dubQ < right ){  //get command before and after double quote
            //cout << "found: double quote" <<endl ;
            string part1 = line.substr(0, dubQ);
            trim(part1);
			result.push_back(part1);
			line = line.substr (dubQ+1);
            if(line.size() ==0){
                perror("paraphraser error");
            }
            string part2 =line.substr(0,  line.find_first_of('\"'));
            trim(part2);
            result.push_back(part2);
			line = line.substr (line.find_first_of('\"')+1);
            //result.at(result.size()-1).insert(0,"\"");
            //result.at(result.size()-1).append("\"");
        }
        else if(pipe < sigQ && pipe < dubQ && pipe < left && pipe < right ){  //get command before pipe
            //cout << "found: pipe"<<endl ;
            string part1 = line.substr(0, pipe);
            trim(part1);
			result.push_back(part1);
			line = line.substr (pipe+1);
            pipeIndex.push_back(result.size());
            result.push_back("|");

        }
        else if(left < sigQ && left < dubQ && left < pipe && left < right ){  //get command before redirection 
            //cout << "found: <"<<endl ;
            string part1 = line.substr(0, left);
            trim(part1);
			result.push_back(part1);
			line = line.substr (left+1);
            leftIndex.push_back(result.size());
            result.push_back("<");
        }
        else if(right < sigQ && right < dubQ && right < pipe && right < left ){  //get command before redirection
            //cout << "found: >"<<endl ;
            string part1 = line.substr(0, right);
            trim(part1);
			result.push_back(part1);
			line = line.substr (right+1);
            rightIndex.push_back(result.size());
            result.push_back(">");
        }
        else{
            trim(line);
			result.push_back (line);
			break;
		}   
    }
    return result;
}
