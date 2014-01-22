#include "Poco/RegularExpression.h"
#include "Poco/DirectoryIterator.h"
#include "Poco/StreamCopier.h"
#include "Poco/Thread.h"
#include "Poco/File.h"
#include "Poco/Exception.h"
#include "Poco/Glob.h"
#include "Poco/DateTime.h"
#include "Poco/TimeSpan.h"
#include <Poco/DateTimeFormatter.h>

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <sstream>

#include <numeric>
#include <algorithm>
using namespace std;

pair<Poco::DateTime, int> AddByVal (const pair<Poco::DateTime, int>& left, const pair<Poco::DateTime, int>& right)
{
	pair<Poco::DateTime, int> ret(left);
	ret.second += right.second;
	return ret;
}

bool CmpByVal (const pair<Poco::DateTime, int>& left, const pair<Poco::DateTime, int>& right)
{
	return left.second>right.second;
}

template <typename T>
T string_to(const std::string & org)
{
	T des = 0;
	std::istringstream ssin(org);

	ssin>>des;
	return des;
}

std::string & str_trim(std::string &str,const std::string &trim_str="\t\r\n\0")
{
	size_t p=str.find_first_not_of(trim_str);
	
	str.erase(0,p);
	
	p=str.find_last_not_of(trim_str);
	
	str.erase(p+1);

	return str;
}

void readYear(int year, std::vector<std::pair<Poco::DateTime, int> >& allMinRainValVec)
{
	std::map<Poco::DateTime, int> allMinRainValMap;
	Poco::DateTime beginDate(year, 1, 1), endDate(year+1, 1, 1);
	std::string yearStr = Poco::DateTimeFormatter::format(beginDate, "%Y");

	while(beginDate < endDate)
	{
		beginDate += Poco::Timespan(60,0);
		allMinRainValMap[beginDate] = 0;
	}


	Poco::RegularExpression rg("^"+yearStr+" ([0-9]{2}) ([0-9]{2})(.+)", Poco::RegularExpression::RE_MULTILINE);
	Poco::RegularExpression::MatchVec matchs;

	std::string content, windstr;
	std::ifstream fin("R015707119832001.DAT");
	Poco::StreamCopier::copyToString(fin, content);

	unsigned pos=0;
	bool isbreak = false;
	while(!isbreak && rg.match(content, pos, matchs) )//提取每行值 包含日期 降水串
	{
		for(int i=0; i<matchs.size();i++)
		{
			//std::cout<<content.substr(matchs[i].offset, matchs[i].length)<<std::endl;
		}

		//year = string_to<int>(content.substr(matchs[1].offset, 4) ),
		int month = string_to<int>(content.substr(matchs[1].offset, 2) ),
			day = string_to<int>(content.substr(matchs[2].offset, 2) );

		Poco::DateTime curDate(year,month,day);
		//std::cout<<Poco::DateTimeFormatter::format(curDate, "\n正在处理 %Y-%m-%d 数据...");

		Poco::RegularExpression rg2(" 0 ([0-9]{4}) ([0-9]{4})(.+?)( [0-9] |$)");
		Poco::RegularExpression::MatchVec matchs2;
		std::string rainStr = content.substr(matchs[3].offset, matchs[3].length);
		unsigned pos2=0;

		while(rg2.match(rainStr, pos2, matchs2) )//提取某日降水串
		{
			//std::cout<<std::endl<<rainStr.substr(matchs2[0].offset, matchs2[0].length);;

			for(int i=0; i<matchs2.size();i++)
			{
				//std::cout<<rainStr.substr(matchs2[i].offset, matchs2[i].length)<<std::endl;
			}
			int beginH = string_to<int>(rainStr.substr(matchs2[1].offset, 2) ),
				beginM = string_to<int>(rainStr.substr(matchs2[1].offset+2, 2) ),
				endH = string_to<int>(rainStr.substr(matchs2[2].offset, 2) ),
				endM = string_to<int>(rainStr.substr(matchs2[2].offset+2, 2) );

			Poco::DateTime nowDate(curDate);
			if(beginH >= 20) nowDate -= Poco::Timespan(1, 0, 0, 0, 0);

			nowDate += Poco::Timespan(0, beginH, beginM, 0, 0);

			std::string mRainVal = rainStr.substr(matchs2[3].offset, matchs2[3].length);



			Poco::RegularExpression rg3(" [0-9]{3}");
			Poco::RegularExpression::MatchVec matchs3;
			unsigned pos3=0;

			while(rg3.match(mRainVal, pos3, matchs3) )//提取某分钟降水
			{
				int val = string_to<int>(mRainVal.substr(matchs3[0].offset, matchs3[0].length) );
				//std::cout<<val<<"----";

				allMinRainValMap[nowDate] = val;
				nowDate += Poco::Timespan(60,0);

				pos3 = matchs3[0].offset+1;
			}

			pos2 = matchs2[0].offset+1;

			//isbreak = true;
		}

		pos = matchs[0].offset+1;

		//std::cout<<Poco::DateTimeFormatter::format(curDate, "\n处理 %Y-%m-%d 数据完毕！");
	}

	//std::cout<<"\nread rain data finish!";

	allMinRainValVec.assign(allMinRainValMap.begin(), allMinRainValMap.end() );
}

void sortSpan(int minuteSpan, 
	const std::vector<std::pair<Poco::DateTime, int> >& allMinRainValVec,
	vector< std::pair<Poco::DateTime, int> >& sortResult)
{
	int n = minuteSpan; 

	sortResult.clear();

	for(std::vector<std::pair<Poco::DateTime, int> >::const_iterator i=allMinRainValVec.begin(); 
		i < allMinRainValVec.end()-n; i++) 
	{
		sortResult.push_back(accumulate(i+1,i+n,*i, AddByVal) );
	}

	sort(sortResult.begin(), sortResult.end(), CmpByVal);

	sortResult.resize(100);
}

void printResult(vector< std::pair<Poco::DateTime, int> >& vec2 , int number)
{

	//cout<<endl;

	//输出前number个数
	for(std::vector<std::pair<Poco::DateTime, int> >::iterator i=vec2.begin();
		// i!=vec2.end(); i++) 
		i < vec2.begin()+number; i++)
	{
		std::cout<<std::endl<<Poco::DateTimeFormatter::format(i->first, "%Y-%m-%d %H:%M : ")<<i->second;
	}
}



void readMonth2(int year, int month, std::map<Poco::DateTime, int>& allMinRainValMap)
{
	Poco::DateTime beginDate(year, month, 1, 20);
	std::string filename = Poco::DateTimeFormatter::format(beginDate, "J57071-%Y%m.TXT");

	std::string content, windstr;
	std::ifstream fin("1983-2013-rain/"+filename);
	if(!fin.is_open() )
	{
		std::cout<<"打开文件"<<filename<<"失败！\n\n";
		return;
	}
	Poco::StreamCopier::copyToString(fin, content);

	//std::cout<<"文件"<<filename<<"长度:"<<content.size()<<"！\n\n";

	beginDate -= Poco::Timespan(1, 0, 0, 0, 0);//设定为前一天20时

	Poco::RegularExpression rg("^R0(.+)^F0", Poco::RegularExpression::RE_MULTILINE|Poco::RegularExpression::RE_DOTALL);
	Poco::RegularExpression::MatchVec matchs;

	if(rg.match(content,0,matchs) )
	{
		for(int i=0; i<matchs.size();i++)
		{
			////std::cout<<content.substr(matchs[i].offset, matchs[i].length)<<std::endl;
			//std::string result = content.substr(matchs[i].offset, matchs[i].length);
			//std::cout<<"match["<<i<<"]长度"<<result.size()<<"！\n\n";
		}

		if(matchs[1].length < 10 ) return;//当月无降水

		std::string lines = content.substr(matchs[1].offset, matchs[1].length);
		str_trim( lines );
		

		unsigned pos1 = 0, 
			pos2 = lines.find_first_of(".", pos1);
		
		while(pos2 != std::string::npos )
		{
			std::string daylines = lines.substr(pos1, pos2-pos1+1);
			str_trim(daylines);
			
			if(daylines.size() == 1) //当天无降水
			{
				beginDate += Poco::Timespan(1, 0, 0, 0, 0);

			}
			else
			{
				unsigned pos11 = 0, pos21 = daylines.find_first_of(",", pos11);
				while(pos21  != std::string::npos )
				{
					std::string hourline = daylines.substr(pos11, pos21-pos11);
					str_trim(hourline);
					
					if(! hourline.size())//本小时无降水
					{
						beginDate += Poco::Timespan(0, 1, 0, 0, 0);
					}
					else
					{
						for(unsigned i=0; i<hourline.size()/2; i++)
						{
							beginDate += Poco::Timespan(60, 0);

							std::string mRainStr = hourline.substr(i*2,2);
							if(mRainStr == "//") continue;

							allMinRainValMap[ beginDate ] = string_to<int>( mRainStr );
						}
						for(unsigned i=hourline.size()/2; i<60; i++)
						{
							beginDate += Poco::Timespan(60, 0);//保证每小时时间同步
						}
					}


					pos11 = pos21 + 1;
					//if(pos11 >= 
					pos21 = daylines.find_first_of(",.=", pos11);
				}
			}

			pos1 = pos2 + 1;
			pos2 = lines.find_first_of(".=", pos1);

		}
	}
}

void readYear2(int year, std::vector<std::pair<Poco::DateTime, int> >& allMinRainValVec)
{
	std::map<Poco::DateTime, int> allMinRainValMap;
	Poco::DateTime beginDate(year, 1, 1), endDate(year+1, 1, 1);
	

	while(beginDate < endDate)
	{
		beginDate += Poco::Timespan(60,0);
		allMinRainValMap[beginDate] = 0;
	}

	for(int month=1; month <=12; month++)
	{
		readMonth2(year, month, allMinRainValMap);
	}

	allMinRainValVec.assign(allMinRainValMap.begin(), allMinRainValMap.end() );
}

void test()
{
	const int length = 9;
	int spans[length] = {5,10,15,20,30,45,60,90,120};

	typedef vector< std::pair<Poco::DateTime, int> > VecType;
	vector<VecType> spanRain(length);
	for(int year=2007; year<=2013; year++)
	{
		VecType yearRain;
		readYear2(year, yearRain);
		std::cout<<"\n\n\n\n\n"<<year<<" 年：";

		for(int i=0; i<length; i++)
		{
			std::cout<<"\n\n "<<spans[i]<<" 分钟排名：";

			VecType sortResult;
			sortSpan(spans[i], yearRain, sortResult);
			printResult(sortResult, 10);

			spanRain[i].insert(spanRain[i].end(), sortResult.begin(), sortResult.end() );
		}



	}

}

int main()
{	
	test();return 0;

	const int length = 9;
	int spans[length] = {5,10,15,20,30,45,60,90,120};

	typedef vector< std::pair<Poco::DateTime, int> > VecType;

	vector<VecType> spanRain(length);
	try
	{
		//test();
		for(int year=1983; year<2006; year++)
		{
			VecType yearRain;
			readYear(year, yearRain);
			std::cout<<"\n\n\n\n\n"<<year<<" 年：";

			for(int i=0; i<length; i++)
			{
				std::cout<<"\n\n "<<spans[i]<<" 分钟排名：";

				VecType sortResult;
				sortSpan(spans[i], yearRain, sortResult);
				printResult(sortResult, 10);

				spanRain[i].insert(spanRain[i].end(), sortResult.begin(), sortResult.end() );
			}



		}

		std::cout<<"\n\n\n\n 所有年份排序：";

		for(int i=0; i<length; i++)
		{
			std::cout<<"\n\n\n "<<spans[i]<<" 分钟排名：";

			sort(spanRain[i].begin(), spanRain[i].end(), CmpByVal);
			printResult(spanRain[i], 10);
		}
	}
	catch(...)
	{
		std::cout<<"\nRegular exception!";
	}

	return 0;
}