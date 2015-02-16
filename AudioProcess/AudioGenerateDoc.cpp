
// AudioGenerateDoc.cpp : implementation of the CAudioGenerateDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "AudioProcess.h"
#endif

#include "AudioGenerateDoc.h"
#include "GenerateDlg.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAudioGenerateDoc

IMPLEMENT_DYNCREATE(CAudioGenerateDoc, CDocument)

BEGIN_MESSAGE_MAP(CAudioGenerateDoc, CDocument)
	ON_COMMAND(ID_GENERATE_FILEOUTPUT, &CAudioGenerateDoc::OnGenerateFileoutput)
	ON_COMMAND(ID_GENERATE_AUDIOOUTPUT, &CAudioGenerateDoc::OnGenerateAudiooutput)
	ON_COMMAND(ID_GENERATE_SINEWAVE, &CAudioGenerateDoc::OnGenerateSinewave)
	ON_COMMAND(ID_GEN_PARAMETERS, &CAudioGenerateDoc::OnGenParameters)
	ON_UPDATE_COMMAND_UI(ID_GENERATE_FILEOUTPUT, &CAudioGenerateDoc::OnUpdateGenerateFileoutput)
	ON_UPDATE_COMMAND_UI(ID_GENERATE_AUDIOOUTPUT, &CAudioGenerateDoc::OnUpdateGenerateAudiooutput)
	ON_COMMAND(ID_GENERATE_MULTIPLESINEWAVES, &CAudioGenerateDoc::OnGenerateMultiplesinewaves)
	ON_COMMAND(ID_HARMONICS_234, &CAudioGenerateDoc::OnHarmonics234)
	ON_COMMAND(ID_HARMONICS_357, &CAudioGenerateDoc::OnHarmonics357)
	ON_COMMAND(ID_HARMONICS_ALL, &CAudioGenerateDoc::OnHarmonicsAll)
	ON_COMMAND(ID_HARMONICS_ODD, &CAudioGenerateDoc::OnHarmonicsOdd)
	ON_COMMAND(ID_HARMONICS_EVEN, &CAudioGenerateDoc::OnHarmonicsEven)
END_MESSAGE_MAP()


/*! Example procedure that generates a sine wave.
 * 
 * The sine wave frequency is set by m_freq1
 */
void CAudioGenerateDoc::OnGenerateSinewave()
{
	// Call to open the generator output
	if(!GenerateBegin())
		return;

	short audio[2];

	for(double time=0.;  time < m_duration;  time += 1. / m_sampleRate)
	{                 
		audio[0] = short(m_amplitude * sin(time * 2 * M_PI * m_freq1));
		audio[1] = short(m_amplitude * sin(time * 2 * M_PI * m_freq1));

		GenerateWriteFrame(audio);

		// The progress control
		if(!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();
}




// CAudioGenerateDoc construction/destruction

CAudioGenerateDoc::CAudioGenerateDoc()
{
    m_freq1 = 440.;
    m_freq2 = 440.;
    m_vibratoamp = 20.;

    m_audiooutput = true;
    m_fileoutput = false;

    m_numChannels = 2;
    m_sampleRate = 44100.;
    m_duration = 10.0;
    m_amplitude = 3276.7;
}

CAudioGenerateDoc::~CAudioGenerateDoc()
{
}

BOOL CAudioGenerateDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}



void CAudioGenerateDoc::OnGenParameters()
{
   CGenerateDlg dlg;

   // Initialize dialog data
   dlg.m_Channels = m_numChannels;
   dlg.m_SampleRate = m_sampleRate;
   dlg.m_Duration = m_duration;
   dlg.m_Amplitude = m_amplitude;
   dlg.m_freq1 = m_freq1;
   dlg.m_freq2 = m_freq2;
   dlg.m_vibratoamp = m_vibratoamp;

   // Invoke the dialog box
   if(dlg.DoModal() == IDOK)
   {
      // Accept the values
      m_numChannels = dlg.m_Channels;
      m_sampleRate = dlg.m_SampleRate;
      m_duration = dlg.m_Duration;
      m_amplitude = dlg.m_Amplitude;
      m_freq1 = dlg.m_freq1;
      m_freq2 = dlg.m_freq2;
      m_vibratoamp = dlg.m_vibratoamp;

      UpdateAllViews(NULL);  
   }
}


#ifdef SHARED_HANDLERS

// Support for thumbnails
void CAudioGenerateDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
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
void CAudioGenerateDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CAudioGenerateDoc::SetSearchContent(const CString& value)
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

// CAudioGenerateDoc diagnostics

#ifdef _DEBUG
void CAudioGenerateDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CAudioGenerateDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CAudioGenerateDoc commands






/////////////////////////////////////////////////////////////////////////////
//
// The following functions manage the audio generation process, 
// directing output to the waveform buffer, file, and/or audio 
// output.  
//
/////////////////////////////////////////////////////////////////////////////


//
// Name :        CAudioGenerateDoc::GenerateBegin()
// Description : This function starts the audio generation process.
//               It opens the waveform storage, opens the file
//               if file output is requested, and opens the 
//               audio output if audio output is requested.
//               Be sure to call EndGenerate() when done.
// Returns :     true if successful...
//

bool CAudioGenerateDoc::GenerateBegin()
{
	// 
	// Waveform storage
	//

	m_waveformBuffer.Start(NumChannels(), SampleRate());

	if(m_fileoutput)
	{
	  if(!OpenGenerateFile(m_wave))
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
// Name :        CAudioGenerateDoc::GenerateWriteFrame()
// Description : Write a frame of output to the current generation device.
// Parameters :  p_frame - An arrays with the number of channels in samples
//               in it.
//

void CAudioGenerateDoc::GenerateWriteFrame(short *p_frame)
{
    m_waveformBuffer.Frame(p_frame);

    if(m_fileoutput)
        m_wave.WriteFrame(p_frame);

    if(m_audiooutput)
        m_soundstream.WriteFrame(p_frame);
}


//
// Name :        CAudioGenerateDoc::GenerateEnd()
// Description : End the generation process.
//

void CAudioGenerateDoc::GenerateEnd()
{
    m_waveformBuffer.End();

    if(m_fileoutput)
        m_wave.close();

    if(m_audiooutput)
        m_soundstream.Close();

    ProgressEnd(this);
}

//
// Name :        CAudioGenerateDoc::OpenGenerateFile()
// Description : This function opens the audio file for output.
// Returns :     true if successful...
//

bool CAudioGenerateDoc::OpenGenerateFile(CWaveOut &p_wave)
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



void CAudioGenerateDoc::OnGenerateFileoutput()
{
   m_fileoutput = !m_fileoutput;
}


void CAudioGenerateDoc::OnGenerateAudiooutput()
{
   m_audiooutput = !m_audiooutput;
}


void CAudioGenerateDoc::OnUpdateGenerateFileoutput(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(m_fileoutput);	
}


void CAudioGenerateDoc::OnUpdateGenerateAudiooutput(CCmdUI *pCmdUI)
{
   pCmdUI->SetCheck(m_audiooutput);	
}



void CAudioGenerateDoc::OnGenerateMultiplesinewaves()
{
	// Call to open the generator output
	if(!GenerateBegin())
		return;

	short audio[2];

	for(double time=0.;  time < m_duration;  time += 1. / m_sampleRate)
	{          
		short sample = short(m_amplitude * (sin(time * 2 * M_PI * m_freq1) + sin(time * 2 * M_PI * m_freq2)));
		audio[0] = sample;
		audio[1] = sample;

		GenerateWriteFrame(audio);

		// The progress control
		if(!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();
}


void CAudioGenerateDoc::OnHarmonics234()
{
	// Call to open the generator output
	if(!GenerateBegin())
		return;

	short audio[2];

	for(double time=0.;  time < m_duration;  time += 1. / m_sampleRate)
	{          

		short sample = short(m_amplitude * sin(time * 2 * M_PI * m_freq1) + 
			                (m_amplitude/2) * sin(time * 2 * M_PI * (2*m_freq1)) + 
							(m_amplitude/3) * sin(time * 2 * M_PI * (3*m_freq1)) + 
							(m_amplitude/4) * sin(time * 2 * M_PI * (4*m_freq1)) );
		audio[0] = sample;
		audio[1] = sample;

		GenerateWriteFrame(audio);

		// The progress control
		if(!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();

}


void CAudioGenerateDoc::OnHarmonics357()
{
	// Call to open the generator output
	if(!GenerateBegin())
		return;

	short audio[2];

	for(double time=0.;  time < m_duration;  time += 1. / m_sampleRate)
	{          

		short sample = short(m_amplitude * sin(time * 2 * M_PI * m_freq1) + 
			                (m_amplitude/3) * sin(time * 2 * M_PI * (3*m_freq1)) + 
							(m_amplitude/5) * sin(time * 2 * M_PI * (5*m_freq1)) + 
							(m_amplitude/7) * sin(time * 2 * M_PI * (7*m_freq1)) );
		audio[0] = sample;
		audio[1] = sample;

		GenerateWriteFrame(audio);

		// The progress control
		if(!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();

}

void CAudioGenerateDoc::OnHarmonicsAll()
{
	// Call to open the generator output
	if(!GenerateBegin())
		return;

	short audio[2];
	int num_harmonics = m_sampleRate /(2* m_freq1);
	
	for(double time=0.;  time < m_duration;  time += 1. / m_sampleRate)
	{          
		long sample = 0;
		for(int i = 1 ; i < num_harmonics; i++ ) 
		{ 
			sample = sample + (m_amplitude/i) * sin(time * 2 * M_PI * (i*m_freq1));
		}
		audio[0] = short(sample);
		audio[1] = short(sample);

		GenerateWriteFrame(audio);

		// The progress control
		if(!GenerateProgress(time / m_duration))
			break;
	}


	// Call to close the generator output
	GenerateEnd();

}

void CAudioGenerateDoc::OnHarmonicsOdd()
{
	// Call to open the generator output
	if(!GenerateBegin())
		return;

	short audio[2];
	int num_harmonics = m_sampleRate / (2*m_freq1);
	
	for(double time=0.;  time < m_duration;  time += 1. / m_sampleRate)
	{          
		long sample = 0;
		for(int i = 1 ; i < num_harmonics; i = i + 2 ) 
		{ 
			sample = sample + (m_amplitude/i) * sin(time * 2 * M_PI * (i*m_freq1));
		}
		audio[0] = short(sample);
		audio[1] = short(sample);

		GenerateWriteFrame(audio);

		// The progress control
		if(!GenerateProgress(time / m_duration))
			break;
	}
	// Call to close the generator output
	GenerateEnd();
}

void CAudioGenerateDoc::OnHarmonicsEven()
{
	// Call to open the generator output
	if(!GenerateBegin())
		return;

	short audio[2];
	int num_harmonics = m_sampleRate / m_freq1;
	
	for(double time=0.;  time < m_duration;  time += 1. / m_sampleRate)
	{          
		long sample = 0;
		for(int i = 2 ; i < num_harmonics; i = i + 2 ) 
		{ 
			sample = sample + (m_amplitude/i) * sin(time * 2 * M_PI * (i*m_freq1));
		}
		audio[0] = short(sample);
		audio[1] = short(sample);

		GenerateWriteFrame(audio);

		// The progress control
		if(!GenerateProgress(time / m_duration))
			break;
	}
	// Call to close the generator output
	GenerateEnd();

}
