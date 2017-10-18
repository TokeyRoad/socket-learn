#include <iostream>
using namespace std;

void printText(char * s){
	cout << s << "+" << endl;
}

void callback(int len, void(*print)(char*), char *s){
	for (int i = 0; i < len; i++){
		print(s);
	}
}

int main(){
	callback(5, printText, "Hello World");
	system("pause");
	return 0;
}
//
//void printwelcome(int len){
//	cout << "欢迎欢迎+" << len << endl;
//}
//
//void printgoodbye(int len){
//	cout << "送客送客+" << len << endl;
//}
//
//void callback(int times, void(*print)(int)){
//	for (int i = 0; i < times; i++){
//		print(i);
//	}
//	cout << "我不知道你是迎客还是送客！" << endl;
//}
//
//int main(){
//	callback(10, printwelcome);
//	callback(10, printgoodbye);
//	printwelcome(5);
//	system("pause");
//	return 0;
//}
