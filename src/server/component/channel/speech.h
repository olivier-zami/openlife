//
// Created by olivier on 09/08/2021.
//

#ifndef ONELIFETEST_SPEECH_H
#define ONELIFETEST_SPEECH_H

namespace server::component::channel
{
	class SpeechService
	{
		public:
			SpeechService();
			~SpeechService();

			void init();

			//!legacy
			int getMaxSpeechPipeIndex();
	};
}

#endif //ONELIFETEST_SPEECH_H
