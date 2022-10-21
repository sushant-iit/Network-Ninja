#include <iostream>
#include <string>

using namespace std;

int getPseudoRandomByte(int S[], int &i, int &j){
    i = (i+1)%256;
    j = (j+S[i])%256;
    swap(S[i], S[j]);
    int k = (S[i] + S[j])%256;
    return S[k];
}

string encrypt_decrypt(string text, string key){

    //Key is <= 256 character long string... Each character can represent value between 0 to 255
    if(key.length() > 256){
        cout << "Key size limit exceeded" << endl;
        exit(1);
    }

    //Initialisation:
    int S[256], T[256];
    for(int i=0; i < 256; i++) 
        S[i] = i;
    for(int i=0; i < 256; i++)
        T[i] = (int)key[i%key.length()];

    // Key Scheduling Algorithm:
    for(int i=0, j = 0; i < 256; i++){
        j = (j + S[i] + T[i])%256;
        swap(S[i], S[j]);
    }

    // Do the encryption/decryption:
    int i=0, j=0;
    string transformedText;
    for(auto ch: text){
        unsigned char temp = ch^(getPseudoRandomByte(S, i, j));
        transformedText += temp;
    }

    return transformedText;
}