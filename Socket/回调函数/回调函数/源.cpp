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
//	cout << "��ӭ��ӭ+" << len << endl;
//}
//
//void printgoodbye(int len){
//	cout << "�Ϳ��Ϳ�+" << len << endl;
//}
//
//void callback(int times, void(*print)(int)){
//	for (int i = 0; i < times; i++){
//		print(i);
//	}
//	cout << "�Ҳ�֪������ӭ�ͻ����Ϳͣ�" << endl;
//}
//
//int main(){
//	callback(10, printwelcome);
//	callback(10, printgoodbye);
//	printwelcome(5);
//	system("pause");
//	return 0;
//}
