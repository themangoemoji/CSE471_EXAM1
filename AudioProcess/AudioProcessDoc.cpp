
// AudioProcessDoc.cpp : implementation of the CAudioProcessDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "AudioProcess.h"
#endif

#include "AudioProcessDoc.h"
#include "ProcessDlg.h"

#include <vector>
#include <fstream>

#include <propkey.h>

using namespace std;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CAudioProcessDoc::OnProcessCopy()
{
   // Call to open the processing output
   if(!ProcessBegin())
      return;

   short audio[2];

   for(int i=0;  i<SampleFrames();  i++)
   {                 
      ProcessReadFrame(audio);

      audio[0] = short(audio[0] * m_amplitude);
      audio[1] = short(audio[1] * m_amplitude);

      ProcessWriteFrame(audio);

      // The progress control
      if(!ProcessProgress(double(i) / SampleFrames()))
         break;
   }

   
   // Call to close the generator output
   ProcessEnd();
}



// CAudioProcessDoc

IMPLEMENT_DYNCREATE(CAudioProcessDoc, CDocument)

BEGIN_MESSAGE_MAP(CAudioProcessDoc, CDocument)
	ON_COMMAND(ID_PROCESS_FILEOUTPUT, &CAudioProcessDoc::OnProcessFileoutput)
	ON_COMMAND(ID_PROCESS_AUDIOOUTPUT, &CAudioProcessDoc::OnProcessAudiooutput)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_FILEOUTPUT, &CAudioProcessDoc::OnUpdateProcessFileoutput)
	ON_UPDATE_COMMAND_UI(ID_PROCESS_AUDIOOUTPUT, &CAudioProcessDoc::OnUpdateProcessAudiooutput)
	ON_COMMAND(ID_PROCESS_COPY, &CAudioProcessDoc::OnProcessCopy)
	ON_COMMAND(ID_PROCESS_PARAMETERS, &CAudioProcessDoc::OnProcessParameters)
	ON_COMMAND(ID_PROCESS_RAMP, &CAudioProcessDoc::OnProcessRamp)
	ON_COMMAND(ID_PROCESS_RAMPIN, &CAudioProcessDoc::OnProcessRampin)
	ON_COMMAND(ID_PROCESS_TREMOLO, &CAudioProcessDoc::OnProcessTremolo)
	ON_COMMAND(ID_PROCESS_HALFSPEED, &CAudioProcessDoc::OnProcessHalfspeed)
	ON_COMMAND(ID_PROCESS_DOUBLESPEED, &CAudioProcessDoc::OnProcessDoublespeed)
	ON_COMMAND(ID_PROCESS_BACKWARDS, &CAudioProcessDoc::OnProcessBackwards)

	ON_COMMAND(ID_EXAM_B3, &CAudioProcessDoc::OnExamB3)
	ON_COMMAND(ID_EXAM_B4, &CAudioProcessDoc::OnExamB4)
	ON_COMMAND(ID_EXAM_B1, &CAudioProcessDoc::OnExamB1)
	ON_COMMAND(ID_EXAM_B2, &CAudioProcessDoc::OnExamB2)
END_MESSAGE_MAP()




// CAudioProcessDoc construction/destruction

CAudioProcessDoc::CAudioProcessDoc()
{
   m_audiooutput = true;
   m_fileoutput = false;

   m_numChannels = 2;
   m_sampleRate = 44100.;
   m_numSampleFrames = 0;
   m_amplitude = 1.0;
}

CAudioProcessDoc::~CAudioProcessDoc()
{
}

BOOL CAudioProcessDoc::OnNewDocument()
{
	return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CAudioProcessDoc serialization

void CAudioProcessDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CAudioProcessDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CAudioProcessDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CAudioProcessDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CAudioProcessDoc diagnostics

#ifdef _DEBUG
void CAudioProcessDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CAudioProcessDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CAudioProcessDoc commands


BOOL CAudioProcessDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	
	if(!m_wavein.Open(lpszPathName))
		return FALSE;

	m_sampleRate = m_wavein.SampleRate();
	m_numChannels = m_wavein.NumChannels();
	m_numSampleFrames = m_wavein.NumSampleFrames();
   
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
//
// The following functions manage the audio processing process, 
// directing output to the waveform buffer, file, and/or audio 
// output.  
//
/////////////////////////////////////////////////////////////////////////////


//
// Name :        CAudioProcessDoc::ProcessBegin()
// Description : This function starts the audio processing process.
//               It opens the waveform storage, opens the file
//               if file output is requested, and opens the 
//               audio output if audio output is requested.
//               Be sure to call EndProcess() when done.
// Returns :     true if successful...
//

bool CAudioProcessDoc::ProcessBegin()
{
    m_wavein.Rewind();

	// 
	// Waveform storage
	//

	m_waveformBuffer.Start(NumChannels(), SampleRate());

    if(m_fileoutput)
    {
      if(!OpenProcessFile(m_waveout))
         return false;
    }

   ProgressBegin(this);

   if(m_audiooutput)
   {
      m_soundstream.SetChannels(NumChannels());
      m_soundstream.SetSampleRate(int(SampleRate()));

      m_soundstream.Open(((CAudioProcessApp *)AfxGetApp())->DirSound());
   }

   return true;
}


//
// Name :        CAudioProcessDoc::ProcessReadFrame()
// Description : Read a frame of input from the current audio file.
//

void CAudioProcessDoc::ProcessReadFrame(short *p_frame)
{
   m_wavein.ReadFrame(p_frame);
}


//
// Name :        CAudioProcessDoc::ProcessWriteFrame()
// Description : Write a frame of output to the current generation device.
//

void CAudioProcessDoc::ProcessWriteFrame(short *p_frame)
{
    m_waveformBuffer.Frame(p_frame);

   if(m_fileoutput)
      m_waveout.WriteFrame(p_frame);

   if(m_audiooutput)
      m_soundstream.WriteFrame(p_frame);
}


//
// Name :        CAudioProcessDoc::ProcessEnd()
// Description : End the generation process.
//

void CAudioProcessDoc::ProcessEnd()
{
    m_waveformBuffer.End();

   if(m_fileoutput)
      m_waveout.close();

   if(m_audiooutput)
      m_soundstream.Close();

   ProgressEnd(this);


}

//
// Name :        CAudioProcessDoc::OpenProcessFile()
// Description : This function opens the audio file for output.
// Returns :     true if successful...
//

bool CAudioProcessDoc::OpenProcessFile(CWaveOut &p_wave)
{
   p_wave.NumChannels(m_numChannels);
   p_wave.SampleRate(m_sampleRate);

	static WCHAR BASED_CODE szFilter[] = L"Wave Files (*.wav)|*.wav|All Files (*.*)|*.*||";

	CFileDialog dlg(FALSE, L".wav", NULL, 0, szFilter, NULL);
	if(dlg.DoModal() != IDOK)
      return false;

    p_wave.open(dlg.GetPathName());
   if(p_wave.fail())
      return false;

   return true;
}



void CAudioProcessDoc::OnProcessFileoutput()
{
	m_fileoutput = !m_fileoutput;
}


void CAudioProcessDoc::OnProcessAudiooutput()
{
   m_audiooutput = !m_audiooutput;
}


void CAudioProcessDoc::OnUpdateProcessFileoutput(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(m_fileoutput);	
}


void CAudioProcessDoc::OnUpdateProcessAudiooutput(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(m_audiooutput);	
}



void CAudioProcessDoc::OnProcessParameters()
{
   CProcessDlg dlg;
   
   dlg.m_amplitude = m_amplitude;

   if(dlg.DoModal() == IDOK)
   {
      m_amplitude = dlg.m_amplitude;
   }
}


void CAudioProcessDoc::OnProcessRamp()
{
 // Call to open the processing output
   if(!ProcessBegin())
      return;

   short audio[2];
   double time = 0;
   double ramp;
   for(int i=0;  i<SampleFrames();  i++, time += 1/ SampleRate())
   {                 
      ProcessReadFrame(audio);
	  if ( time < 0.5)
	  {
		  ramp = time / 0.5;
	  }
	  else
	  {
		  ramp = 1;
	  }
      audio[0] = short(audio[0] * m_amplitude * ramp);
      audio[1] = short(audio[1] * m_amplitude * ramp);

      ProcessWriteFrame(audio);

      // The progress control
      if(!ProcessProgress(double(i) / SampleFrames()))
         break;
   }

   
   // Call to close the generator output
   ProcessEnd();
}


void CAudioProcessDoc::OnProcessRampin()
{
	// Call to open the processing output
   if(!ProcessBegin())
      return;

   short audio[2];
   double time = 0;
   double ramp;
   double duration = SampleFrames()/SampleRate();
   for(int i=0;  i<SampleFrames();  i++, time += 1/ SampleRate())
   {                 
      ProcessReadFrame(audio);
	  if ( time < 1.5)
	  {
		  ramp = 2;
	  }
	  else if ( time > (duration -1.5) )
	  {
		  ramp = 0.5;
	  }
	  else
	  {
		  ramp = 1;
	  }
      audio[0] = short(audio[0] * m_amplitude * ramp);
      audio[1] = short(audio[1] * m_amplitude * ramp);

      ProcessWriteFrame(audio);

      // The progress control
      if(!ProcessProgress(double(i) / SampleFrames()))
         break;
   }

   
   // Call to close the generator output
   ProcessEnd();
}


void CAudioProcessDoc::OnProcessTremolo()
{
   // Call to open the processing output
   if(!ProcessBegin())
      return;

   short audio[2];
   double time = 0;
   double tremolo_depth = 0.2;
   double tremolo_freq = 4;
   double mult_factor;

   double duration = SampleFrames()/SampleRate();
   for(int i=0;  i<SampleFrames();  i++, time += 1/ SampleRate())
   {                 
      ProcessReadFrame(audio);
	  
	  mult_factor = 1 +  tremolo_depth*sin(tremolo_freq * 2* M_PI * time);
	  audio[0] = short(audio[0] * m_amplitude * mult_factor);
      audio[1] = short(audio[1] * m_amplitude * mult_factor);

      ProcessWriteFrame(audio);

      // The progress control
      if(!ProcessProgress(double(i) / SampleFrames()))
         break;
   }

   
   // Call to close the generator output
   ProcessEnd();
}


void CAudioProcessDoc::OnProcessHalfspeed()
{
   // Call to open the processing output
   if(!ProcessBegin())
      return;

   short audio[2];
   short prev_sample = 0;
   
   for(int i=0;  i<SampleFrames();  i++)
   {                 
      ProcessReadFrame(audio);
	  short sample = audio[0];
	  if (i >= 1 )
	  {
		  audio[0] = short(0.5*(sample + prev_sample) * m_amplitude);
		  audio[1] = short(0.5*(sample + prev_sample) * m_amplitude);

		  ProcessWriteFrame(audio);
	  }

	  prev_sample = sample;
	  audio[0] = short(sample * m_amplitude);
      audio[1] = short(sample * m_amplitude);
	  

      ProcessWriteFrame(audio);

	  
      // The progress control
      if(!ProcessProgress(double(i) / SampleFrames()))
         break;
   }

   
   // Call to close the generator output
   ProcessEnd();
	
}


void CAudioProcessDoc::OnProcessDoublespeed()
{
	 // Call to open the processing output
   if(!ProcessBegin())
      return;

   short audio[2];
   short prev_sample = 0;
   short sample = 0;
   int i;
   if (SampleFrames() % 2 == 0)
	   i = 0;
   else
	   i = 1;

   while(i<SampleFrames())
   {
	  ProcessReadFrame(audio);
	  prev_sample = audio[0];

      ProcessReadFrame(audio);
	  sample = audio[0];

	  audio[0] = short(0.5*(sample + prev_sample) * m_amplitude);
	  audio[1] = short(0.5*(sample + prev_sample) * m_amplitude);
		  
      ProcessWriteFrame(audio);	  

      // The progress control
      if(!ProcessProgress(double(i) / SampleFrames()))
         break;
	  
	  i = i + 2;
   }

   // Call to close the generator output
   ProcessEnd();
}


void CAudioProcessDoc::OnProcessBackwards()
{
	// Call to open the processing output
   if(!ProcessBegin())
      return;

   short audio[2];
   short *waveTable = new short[SampleFrames() + 1];
   
   for(int i = 0; i<SampleFrames(); i++)
   {
	  ProcessReadFrame(audio);
	  waveTable[i] = audio[0];

   }
   for(int i = SampleFrames()-1; i >= 0; i--)
   {
      	  
	  audio[0] = short(waveTable[i] * m_amplitude);
	  audio[1] = short(waveTable[i] * m_amplitude);
	  ProcessWriteFrame(audio);
	  // The progress control
      if(!ProcessProgress(double(SampleFrames() - i) / SampleFrames()))
         break;
	
   }
   delete(waveTable);
   // Call to close the generator output
   ProcessEnd();
}






void CAudioProcessDoc::OnExamB3()
{
	// Call to open the processing output
	if (!ProcessBegin())
		return;

	short audio[2];
	short prev_sample = 0;
	double rate = 0.0;
	double sample = 0.0;
	double time = 0.0;
	int i = 0;
	vector<short> wavetable;
	for (int j = 0; j<SampleFrames(); j++){
		ProcessReadFrame(audio);
		wavetable.push_back(audio[0]);
	}

	while (i<SampleFrames())
	{
		rate = (1 + 0.1 * cos(M_PI * (time / (SampleFrames() / SampleRate()))));
		//rate = 1 + 0.1*sin(2 * M_PI * 3 * time);
		sample += rate;
		audio[0] = wavetable[i];
		ProcessWriteFrame(audio);
		i = int(sample + 0.5);
		time += 1. / SampleRate();
		// The progress control
		if (!ProcessProgress(double(i) / SampleFrames()))
			break;
	}

	// Call to close the generator output
	ProcessEnd();
}


void CAudioProcessDoc::OnExamB4()
{
	// Call to open the processing output
	if (!ProcessBegin())
		return;

	short audio[2];
	short prev_sample = 0;
	double rate = 0.0;
	double sample = 0.0;
	double time = 0.0;
	int i = 0;
	vector<short> wavetable;
	for (int j = 0; j<SampleFrames(); j++){
		ProcessReadFrame(audio);
		wavetable.push_back(audio[0]);
	}

	while (i<SampleFrames())
	{
		rate = 1 + 0.1*sin(2 * M_PI * 3 * time);
		sample += rate;
		audio[0] = wavetable[i];
		ProcessWriteFrame(audio);
		i = int(sample + 0.5);
		time += 1. / SampleRate();
		// The progress control
		if (!ProcessProgress(double(i) / SampleFrames()))
			break;
	}

	// Call to close the generator output
	ProcessEnd();
}


void CAudioProcessDoc::OnExamB1()
{
	// Call to open the processing output
	if (!ProcessBegin())
		return;

	short audio[2];

	const int QUEUESIZE = 200000;
	const double DELAY = 1.0;

	std::vector<short> queue;
	queue.resize(QUEUESIZE);

	int wrloc = 0;

	double time = 0;

	for (int i = 0; i<SampleFrames(); i++, time += 1. / SampleRate())
	{
		ProcessReadFrame(audio);

		wrloc = (wrloc + 2) % QUEUESIZE;
		queue[wrloc] = audio[0];
		queue[wrloc + 1] = audio[1];

		int delaylength = int((0.006 + sin(0.25 * 2 * M_PI * time) * 0.004 * SampleRate() + 0.5)) * 2;  //**/// this line ----!
		int rdloc = (wrloc + QUEUESIZE - delaylength) % QUEUESIZE;

		audio[0] = audio[0] / 2 + queue[rdloc++] / 2;
		audio[1] = audio[1] / 2 + queue[rdloc] / 2;

		ProcessWriteFrame(audio);

		// The progress control
		if (!ProcessProgress(double(i) / SampleFrames()))
			break;
	}


	// Call to close the generator output
	ProcessEnd();
}


void CAudioProcessDoc::OnExamB2()
{
	// Call to open the processing output
	if (!ProcessBegin())
		return;

	short audio[2];

	const int QUEUESIZE = 200000;
	const double DELAY = 1.0;

	std::vector<short> queue;
	queue.resize(QUEUESIZE);

	int wrloc = 0;

	double time = 0;

	for (int i = 0; i<SampleFrames(); i++, time += 1. / SampleRate())
	{
		ProcessReadFrame(audio);

		wrloc = (wrloc + 2) % QUEUESIZE;
		queue[wrloc] = audio[0];
		queue[wrloc + 1] = audio[1];

		int delaylength = int((0.025 + sin(0.25 * 2 * M_PI * time) * 0.005 * SampleRate() + 0.5)) * 2; /// this line--------!
		int rdloc = (wrloc + QUEUESIZE - delaylength) % QUEUESIZE;

		audio[0] = audio[0] / 2 + queue[rdloc++] / 2;
		audio[1] = audio[1] / 2 + queue[rdloc] / 2;

		ProcessWriteFrame(audio);

		// The progress control
		if (!ProcessProgress(double(i) / SampleFrames()))
			break;
	}


	// Call to close the generator output
	ProcessEnd();
}
