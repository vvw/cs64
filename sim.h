

/*
The simplest UI program
Select Visual C++ CLR and CLR Empty Project
and type in RandomNumberGenerator for the project name. The, OK.
Project->Add New Item... .
Select UI under Visual C++.
Leave the Form name as given by default MyForm.h.
Then, click Add.
We need to edit the MyForm.cpp file:
#include "MyForm.h"
using namespace System;
using namespace System::Windows::Forms;
[STAThread]
void Main(array<String^>^ args)
{
Application::EnableVisualStyles();
Application::SetCompatibleTextRenderingDefault(false);
RandomNumberGenerator::MyForm form;
Application::Run(%form);
}

The System namespace provides functions to work with UI controls.
At the right-mouse click on RandomNumberGenerator, we get the Properties window.
Configuration Properties->Linker->System
Select Windows (/SUBSYSTEM:WINDOWS) for SubSystem.
Advanced->Entry Point, type in Main.
The, hit OK.
*/

// visual studio 源文件utf-8 编码必须要有BOM 才行
// http://www.unicode.org/cgi-bin/GetUnihanData.pl?codepoint=%E4%B8%A5
// https://www.sqlite.org/c3ref/create_function.html
// https://github.com/schuyler/levenshtein

#pragma once

#include "windows.h"
#include < stdio.h >
#include < stdlib.h >
#include <vcclr.h>
#include <string>
#include <iostream>
using namespace std;
using namespace System;

//  Convert System::String to wchar_t* or char*
void testConvertString() {
	String ^str = "Hello";

	// Pin memory so GC can't move it while native function is called
	pin_ptr<const wchar_t> wch = PtrToStringChars(str);
	printf_s("%S\n", wch);

	// Conversion to char* :
	// Can just convert wchar_t* to char* using one of the 
	// conversion functions such as: 
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	size_t convertedChars = 0;
	size_t  sizeInBytes = ((str->Length + 1) * 2);
	errno_t err = 0;
	char    *ch = (char *)malloc(sizeInBytes);

	err = wcstombs_s(&convertedChars,
		ch, sizeInBytes,
		wch, sizeInBytes);
	if (err != 0)
		printf_s("wcstombs_s  failed!\n");

	printf_s("%s\n", ch);
}

void MarshalString(String ^ s, string& os) {
	using namespace Runtime::InteropServices;
	const char* chars =
		(const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer();
	os = chars;
	Marshal::FreeHGlobal(IntPtr((void*)chars));
}

void MarshalString(String ^ s, wstring& os) {
	using namespace Runtime::InteropServices;
	const wchar_t* chars =
		(const wchar_t*)(Marshal::StringToHGlobalUni(s)).ToPointer();
	os = chars;
	Marshal::FreeHGlobal(IntPtr((void*)chars));
}

void testMarshalString() {
	string a = "test";
	wstring b = L"test2";
	String ^ c = gcnew String("abcd");

	cout << a << endl;
	MarshalString(c, a);
	c = "efgh";
	MarshalString(c, b);
	cout << a << endl;
	wcout << b << endl;
}

namespace Project1 {


	// visual studio 源文件utf-8 编码必须要有BOM 才行
	// http://www.unicode.org/cgi-bin/GetUnihanData.pl?codepoint=%E4%B8%A5
	// https://www.sqlite.org/c3ref/create_function.html
	// https://github.com/schuyler/levenshtein

# define min(x, y) ((x) < (y) ? (x) : (y))
# define max(x, y) ((x) > (y) ? (x) : (y))

	// 某个utf8 字符占几个字节
	// c: 必须指向utf8 字符串
	static int utf8len(char *c) {
		unsigned char c1 = c[0];
		int len = -1;
		if ((c1 & 0x80) == 0) {  // 0b10000000
			len = 1;
		}
		else if ((c1 & 0xF0) == 0xF0) {  // 0b11110000
			len = 4;
		}
		else if ((c1 & 0xE0) == 0xE0) {  // 0b11100000
			len = 3;
		}
		else if ((c1 & 0xC0) == 0xC0) {  // 0b11000000 
			len = 2;
		}
		else {
			return -1;
		}
		return len;
	}

	/*
	** Assuming z points to the first byte of a UTF-8 character,
	** advance z to point to the first byte of the next UTF-8 character.
	*/
	// 计算字符个数
	// 实现参考sqlite3 的lengthFunc 函数
	static int utf8strlen(char *str) {
		int len;
		const unsigned char *z = (const unsigned char *)str;
		if (z == 0) {
			return -1;
		}
		len = 0;
		while (*z){
			len++;
			//SQLITE_SKIP_UTF8(z);
			if ((*(z++)) >= 0xc0) {
				while ((*z & 0xc0) == 0x80){ z++; }
			}
		}
		return len;
	}

	// utf8 编码规则
	/*
	1字节 0xxxxxxx
	2字节 110xxxxx 10xxxxxx 0xC0 0x80
	3字节 1110xxxx 10xxxxxx 10xxxxxx
	4字节 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	5字节 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	6字节 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
	*/
	// 假定z 指向第一个utf8 字符，函数执行完以后z 指向下一个字符
	static char *nextc(char *z) {
		if (z == 0) { return 0; }
		if (*z == 0) {
			return 0;
		}
		++z;
		while ((*z & 0xC0) == 0x80) { ++z; }  // 只要最高位是10 开头就继续移动指针
		return z;
	}

	static char *at(char *z, int pos) {
		char *t = z;
		int i;
		for (i = 0; i < pos; i++) {
			t = nextc(t);
		}
		return t;
	}

	static int utf8eq(char *c1, char *c2) {
		int i;
		if (c1 == 0 || c2 == 0 || *c1 == 0 || *c2 == 0) {
			return -1;
		}
		int len1 = utf8len(c1);
		int len2 = utf8len(c2);
		if (len1 != len2) {
			return 0;
		}
		else {
			for (i = 0; i < len1; i++) {
				if (c1[i] != c2[i]) {
					return 0;
				}
			}
		}
		return 1;
	}

	static unsigned int levenshtein(char *word1_in,  char *word2_in) {
		 char *word1 = word1_in;
		 char *word2 = word2_in;
		int len1 = utf8strlen(word1),
			len2 = utf8strlen(word2);
		int *v = (int *)calloc(len2 + 1, sizeof(unsigned int));
		 int i, j, current, next, cost;

		/* strip common prefixes */
		while (len1 > 0 && len2 > 0 && utf8eq(word1, word2)) {
			word1 = nextc(word1);
			word2 = nextc(word2);
			len1--;
			len2--;
		}

		/* handle degenerate cases */
		if (!len1) return len2;
		if (!len2) return len1;

		/* initialize the column vector */
		for (j = 0; j < len2 + 1; j++)
			v[j] = j;

		for (i = 0; i < len1; i++) {
			/* set the value of the first row */
			current = i + 1;
			/* for each row in the column, compute the cost */
			for (j = 0; j < len2; j++) {
				/*
				* cost of replacement is 0 if the two chars are the same, or have
				* been transposed with the chars immediately before. otherwise 1.
				*/
				cost = !(utf8eq(at(word1, i), at(word2, j)) || (i && j &&
					utf8eq(at(word1, i - 1), at(word2, j)) && utf8eq(at(word1, i), at(word2, j - 1))));
				/* find the least cost of insertion, deletion, or replacement */
				next = min(min(v[j + 1] + 1,
					current + 1),
					v[j] + cost);
				/* stash the previous row's cost in the column vector */
				v[j] = current;
				/* make the cost of the next transition current */
				current = next;
			}
			/* keep the final cost at the bottom of the column */
			v[len2] = next;
		}
		free(v);
		return next;
	}

	__declspec(dllexport) double __stdcall sim(char *word1, char *word2)  {
		int len1 = utf8strlen(word1);
		int len2 = utf8strlen(word2);
		int len = max(len1, len2);
		if (len == 0) {
			return -1;
		}
		int distance = levenshtein(word1, word2);
		//return distance;
		return 1 - distance / (double)len;
	}

	__declspec(dllexport) int __stdcall add(int a, int b) {
		return a + b;
	}

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Text;

	/// <summary>
	/// Summary for MyForm
	/// </summary>
	public ref class MyForm : public System::Windows::Forms::Form
	{
	public:
		MyForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::RichTextBox^  richTextBox1;
	private: System::Windows::Forms::RichTextBox^  richTextBox2;
	protected:

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->richTextBox1 = (gcnew System::Windows::Forms::RichTextBox());
			this->richTextBox2 = (gcnew System::Windows::Forms::RichTextBox());
			this->SuspendLayout();
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(347, 506);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(170, 62);
			this->button1->TabIndex = 0;
			this->button1->Text = L"button1";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &MyForm::button1_Click);
			// 
			// richTextBox1
			// 
			this->richTextBox1->Location = System::Drawing::Point(12, 12);
			this->richTextBox1->Name = L"richTextBox1";
			this->richTextBox1->Size = System::Drawing::Size(780, 248);
			this->richTextBox1->TabIndex = 1;
			this->richTextBox1->Text = L"";
			// 
			// richTextBox2
			// 
			this->richTextBox2->Location = System::Drawing::Point(12, 282);
			this->richTextBox2->Name = L"richTextBox2";
			this->richTextBox2->Size = System::Drawing::Size(780, 209);
			this->richTextBox2->TabIndex = 2;
			this->richTextBox2->Text = L"";
			// 
			// MyForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(828, 595);
			this->Controls->Add(this->richTextBox2);
			this->Controls->Add(this->richTextBox1);
			this->Controls->Add(this->button1);
			this->Name = L"MyForm";
			this->Text = L"MyForm";
			this->ResumeLayout(false);

		}
#pragma endregion


	public: char *newArray(String ^ s) {
			array<unsigned char, 1> ^a = System::Text::UTF8Encoding().GetBytes(s);
			char *p = (char *)malloc(a->Length+1);
			for (int i = 0; i < a->Length; i++) {
				p[i] = a[i];
			}
			p[a->Length] = 0;
			return p;
		}

			static Encoding^ GBKEndoding()
			{
				return Encoding::GetEncoding("gb18030");
			}
			static Encoding^ Utf8Endoding()
			{
				return Encoding::GetEncoding("utf-8");
			}
			static String^ gbk2utf8(String^ s) {
				return Utf8Endoding()->GetString(Encoding::Convert(GBKEndoding(), Utf8Endoding(), GBKEndoding()->GetBytes(s)));
			}

	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {

				 String ^ s1 = this->richTextBox1->Text;
				 String ^ s2 = this->richTextBox2->Text;
				 if (s1->Length <= 0 || s2->Length <= 0) {
					 System::Windows::Forms::MessageBox::Show(L"请输入字符串！");
					 return;
				 }
				 s1 = gbk2utf8(s1);
				 s2 = gbk2utf8(s2);
				 array<unsigned char, 1> ^a = System::Text::UTF8Encoding().GetBytes(s1);
				 char *p = newArray(s1);
				 char *p2 = newArray(s2);
				 int len1 = utf8strlen(p);
				 int len2 = utf8strlen(p2);
				 //String ^sp = System::String(p).ToString();
				 double similar = sim(p, p2);
				 System::Windows::Forms::MessageBox::Show(L"相似度：" + similar.ToString());
				 free(p);
				 free(p2);
	}

	};
}
