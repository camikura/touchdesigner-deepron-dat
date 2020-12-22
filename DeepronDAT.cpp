/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/

#include "CPlusPlus_Common.h"
#include "DAT_CPlusPlusBase.h"

#include <stdio.h>
#include <array>
#include <string>
#include <thread>
#include <regex>

#include "Serial.hpp"

using namespace std;

class DeepronDAT : public DAT_CPlusPlusBase
{
public:
	std::string portname = "";

	Serial serial;
	std::thread recv_thread;
	bool running;

	string recv_string;
	smatch smatch;
	array<string, 2> result;

	DeepronDAT(const OP_NodeInfo* info)
	{
		recv_string = "";

		this->running = false;
		this->reset();
	}

	~DeepronDAT()
	{
		this->stop();
		this->close();
	}

	void reset()
	{
		result = { "00'00\"00000", "00'00\"00000" };
	}

	bool open()
	{
		// open error
		if (!this->serial.Open(this->portname)) {
			return false;
		}

		cout << "open success" << endl;

		return true;
	}

	void start()
	{
		std::cout << "thread start" << std::endl;

		recv_thread = std::thread([this]() {
			this->loop();
		});
	}

	void loop()
	{
		this->running = true;

		while (this->running)
		{

			auto vals = this->serial.Read();
			for (auto c : vals) {
				recv_string += c;

				std::regex re("<ID0([\\d])><([^>]+)><E>");
				if (regex_search(recv_string, smatch, re))
				{
					int index = stoi(smatch[1]) - 1;
					if (index < result.size()) {
						result[index] = smatch[2];
					}

					recv_string = "";
				}

			}
		}
	}

	void stop()
	{
		std::cout << "thread stop" << std::endl;

		this->running = false;
		if (recv_thread.joinable())
			recv_thread.join();
	}

	void close()
	{
		this->serial.Close();
	}

	void getGeneralInfo(DAT_GeneralInfo* ginfo, const OP_Inputs* inputs, void* reserved1)
	{
		ginfo->cookEveryFrameIfAsked = true;
	}

	void execute(DAT_Output* output, const OP_Inputs* inputs, void* reserved)
	{
		output->setOutputDataType(DAT_OutDataType::Table);
		output->setTableSize(2, 1);

		for (int i = 0; i < result.size(); i++) {
			output->setCellString(i, 0, result[i].c_str());
		}

		std::string name = inputs->getParString("Portname");

		if (this->portname != name) {
			this->stop();
			this->close();
		}
		this->portname = name;

		if (!this->portname.size())
			return;

		if (!this->running) {
			if (!this->open())
				return;
			this->start();
		}
	}

	void setupParameters(OP_ParameterManager* manager, void* reserved1)
	{
		{
			OP_StringParameter	sp;
			sp.name = "Portname";
			sp.label = "Port Name";
			OP_ParAppendResult res = manager->appendString(sp);
			assert(res == OP_ParAppendResult::Success);
		}

		{
			OP_NumericParameter	np;
			np.name = "Reset";
			np.label = "Reset";
			OP_ParAppendResult res = manager->appendPulse(np);
			assert(res == OP_ParAppendResult::Success);
		}

	}

	void pulsePressed(const char* name, void* reserved1)
	{
		if (!strcmp(name, "Reset"))
			this->reset();
	}
};

extern "C"
{
	DLLEXPORT void FillDATPluginInfo(DAT_PluginInfo* info)
	{
		info->apiVersion = DATCPlusPlusAPIVersion;
		info->customOPInfo.opType->setString("Deepron");
		info->customOPInfo.opLabel->setString("Deepron");
		info->customOPInfo.authorName->setString("Akira Kamikura");
		info->customOPInfo.authorEmail->setString("akira.kamikura@gmail.com");
	}

	DLLEXPORT DAT_CPlusPlusBase* CreateDATInstance(const OP_NodeInfo* info)
	{
		return new DeepronDAT(info);
	}

	DLLEXPORT void DestroyDATInstance(DAT_CPlusPlusBase* instance)
	{
		delete (DeepronDAT*)instance;
	}
};
