#include<bits/stdc++.h>
#include "include/rapidjson/document.h"
#include "include/rapidjson/writer.h"
#include "include/rapidjson/stringbuffer.h"


int main(){
    std::ifstream t("./linwt.json");
    std::string str((std::istreambuf_iterator<char>(t)),std::istreambuf_iterator<char>());
    const char * c = str.c_str();
    rapidjson::Document d;
    d.Parse(c);
    rapidjson::Value& a = d["weights"];
    float *wt= (float *)malloc(sizeof(float)*a.Size());
    float *bs= (float *)malloc(sizeof(float)*a.Size());

    for (rapidjson::SizeType i = 0; i < a.Size(); i++) {
         wt[i]=a[i].GetDouble();
    }
    a = d["bias"];
    for (rapidjson::SizeType i = 0; i < a.Size(); i++) {
        // printf("a[%d] = %lf\n", i, a[i].GetDouble());
        bs[i]=a[i].GetDouble();
    }
}