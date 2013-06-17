#include "BackgroundSubtractorLBSP.h"
#include "LBSP.h"
#include "DistanceUtils.h"
#include "RandUtils.h"
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

BackgroundSubtractorLBSP::BackgroundSubtractorLBSP()
	:	 m_nBGSamples(BGSLBSP_DEFAULT_NB_BG_SAMPLES)
		,m_nRequiredBGSamples(BGSLBSP_DEFAULT_REQUIRED_NB_BG_SAMPLES)
		,m_voBGImg(BGSLBSP_DEFAULT_NB_BG_SAMPLES)
		,m_voBGDesc(BGSLBSP_DEFAULT_NB_BG_SAMPLES)
	 	,m_nDescDistThreshold(BGSLBSP_DEFAULT_DESC_DIST_THRESHOLD)
		,m_nColorDistThreshold(LBSP_DEFAULT_ABS_SIMILARITY_THRESHOLD)
		,m_bInitialized(false)
		,m_oExtractor(LBSP_DEFAULT_ABS_SIMILARITY_THRESHOLD) {
	CV_Assert(m_nBGSamples>0);
}

BackgroundSubtractorLBSP::BackgroundSubtractorLBSP(  int nLBSPThreshold
													,int nDescDistThreshold
													,int nBGSamples
													,int nRequiredBGSamples)
	:	 m_nBGSamples(nBGSamples)
		,m_nRequiredBGSamples(nRequiredBGSamples)
		,m_voBGImg(nBGSamples)
		,m_voBGDesc(nBGSamples)
		,m_nDescDistThreshold(nDescDistThreshold)
		,m_nColorDistThreshold(nLBSPThreshold)
		,m_bInitialized(false)
		,m_oExtractor(nLBSPThreshold) {
	CV_Assert(m_nBGSamples>0);
}

BackgroundSubtractorLBSP::BackgroundSubtractorLBSP(	 float fLBSPThreshold
													,int nDescDistThreshold
													,int nBGSamples
													,int nRequiredBGSamples)
	:	 m_nBGSamples(nBGSamples)
		,m_nRequiredBGSamples(nRequiredBGSamples)
		,m_voBGImg(nBGSamples)
		,m_voBGDesc(nBGSamples)
		,m_nDescDistThreshold(nDescDistThreshold)
		,m_nColorDistThreshold(LBSP_DEFAULT_ABS_SIMILARITY_THRESHOLD) // @@@@@@@@@ not using relative...
		,m_bInitialized(false)
		,m_oExtractor(fLBSPThreshold) {
	CV_Assert(m_nBGSamples>0);
}

BackgroundSubtractorLBSP::~BackgroundSubtractorLBSP() {}

void BackgroundSubtractorLBSP::initialize(const cv::Mat& oInitImg) {
	CV_Assert(!oInitImg.empty() && oInitImg.cols>0 && oInitImg.rows>0);
	CV_Assert(oInitImg.type()==CV_8UC1 || oInitImg.type()==CV_8UC3);
	m_oImgSize = oInitImg.size();
	m_nImgType = oInitImg.type();
	m_nImgChannels = oInitImg.channels();

	// init keypoints used for the extractor :
	cv::DenseFeatureDetector oKPDDetector(1.f, 1, 1.f, 1, 0, true, false);
	if(m_voKeyPoints.capacity()<(size_t)(m_oImgSize.width*m_oImgSize.height))
		m_voKeyPoints.reserve(m_oImgSize.width*m_oImgSize.height);
	oKPDDetector.detect(cv::Mat(m_oImgSize,m_nImgType), m_voKeyPoints);
	LBSP::validateKeyPoints(m_voKeyPoints,m_oImgSize);
	CV_Assert(!m_voKeyPoints.empty());

	// init bg model samples :
	cv::Mat oInitDesc;
	m_oExtractor.compute2(oInitImg,m_voKeyPoints,oInitDesc);
	CV_Assert(m_voBGImg.size()==(size_t)m_nBGSamples);
	const int nKeyPoints = (int)m_voKeyPoints.size();
	int y_sample, x_sample;
	if(m_nImgChannels==1) {
		for(int s=0; s<m_nBGSamples; s++) {
			m_voBGImg[s].create(m_oImgSize,m_nImgType);
			m_voBGDesc[s].create(m_oImgSize,CV_16UC1);
			for(int k=0; k<nKeyPoints; ++k) {
				const int y_orig = (int)m_voKeyPoints[k].pt.y;
				const int x_orig = (int)m_voKeyPoints[k].pt.x;
				getRandSamplePosition(x_sample,y_sample,x_orig,y_orig,LBSP::PATCH_SIZE/2,m_oImgSize);
				m_voBGImg[s].at<uchar>(y_orig,x_orig) = oInitImg.at<uchar>(y_sample,x_sample);
				m_voBGDesc[s].at<unsigned short>(y_orig,x_orig) = oInitDesc.at<unsigned short>(y_sample,x_sample);
			}
		}
	}
	else { //m_nImgChannels==3
		for(int s=0; s<m_nBGSamples; s++) {
			m_voBGImg[s].create(m_oImgSize,m_nImgType);
			m_voBGDesc[s].create(m_oImgSize,CV_16UC3);
			m_voBGImg[s] = cv::Scalar_<uchar>(0,0,0);
			m_voBGDesc[s] = cv::Scalar_<ushort>(0,0,0);
			for(int k=0; k<nKeyPoints; ++k) {
				const int y_orig = (int)m_voKeyPoints[k].pt.y;
				const int x_orig = (int)m_voKeyPoints[k].pt.x;
				getRandSamplePosition(x_sample,y_sample,x_orig,y_orig,LBSP::PATCH_SIZE/2,m_oImgSize);
				const int idx_orig_img = oInitImg.step.p[0]*y_orig + oInitImg.step.p[1]*x_orig;
				const int idx_orig_desc = oInitDesc.step.p[0]*y_orig + oInitDesc.step.p[1]*x_orig;
				const int idx_rand_img = oInitImg.step.p[0]*y_sample + oInitImg.step.p[1]*x_sample;
				const int idx_rand_desc = oInitDesc.step.p[0]*y_sample + oInitDesc.step.p[1]*x_sample;
				uchar* bgimg_ptr = m_voBGImg[s].data+idx_orig_img;
				const uchar* initimg_ptr = oInitImg.data+idx_rand_img;
				unsigned short* bgdesc_ptr = (unsigned short*)(m_voBGDesc[s].data+idx_orig_desc);
				const unsigned short* initdesc_ptr = (unsigned short*)(oInitDesc.data+idx_rand_desc);
				for(int n=0;n<3; ++n) {
					bgimg_ptr[n] = initimg_ptr[n];
					bgdesc_ptr[n] = initdesc_ptr[n];
				}
			}
		}
	}
	m_bInitialized = true;
}

void BackgroundSubtractorLBSP::operator()(cv::InputArray _image, cv::OutputArray _fgmask, double learningRate) {
	CV_DbgAssert(m_bInitialized);
	CV_DbgAssert(learningRate>0);
	cv::Mat oInputImg = _image.getMat(), oInputDesc;
	CV_DbgAssert(oInputImg.type()==m_nImgType && oInputImg.size()==m_oImgSize);
	_fgmask.create(m_oImgSize,CV_8UC1);
	cv::Mat oFGMask = _fgmask.getMat();
	oFGMask = cv::Scalar_<uchar>(0);
	const int nKeyPoints = (int)m_voKeyPoints.size();
	const int nLearningRate = (int)learningRate;
	if(m_nImgChannels==1) {
		// note: modulation is commented here since noise doesn't affect the pattern that much, considering we use the modulated color threshold to build it
		const int nCurrDescDistThreshold = m_nDescDistThreshold/**LBSP_SINGLECHANNEL_THRESHOLD_MODULATION_FACT*/;
		const int nCurrColorDistThreshold = m_nColorDistThreshold*LBSP_SINGLECHANNEL_THRESHOLD_MODULATION_FACT;
		unsigned short nCurrInputDesc;
		for(int k=0; k<nKeyPoints; ++k) {
			const int x = (int)m_voKeyPoints[k].pt.x;
			const int y = (int)m_voKeyPoints[k].pt.y;
			int nGoodSamplesCount=0, nSampleIdx=0;
			while(nGoodSamplesCount<m_nRequiredBGSamples && nSampleIdx<m_nBGSamples) {
				LBSP::computeSingle(oInputImg,m_voBGImg[nSampleIdx],x,y,nCurrColorDistThreshold,nCurrInputDesc);
				if(hdist_ushort_8bitLUT(nCurrInputDesc,m_voBGDesc[nSampleIdx].at<unsigned short>(y,x))<=nCurrDescDistThreshold) {
					const int idx_img = oInputImg.step.p[0]*y + x;
					CV_DbgAssert(*(oInputImg.data+idx_img)==oInputImg.at<uchar>(y,x));
					if(absdiff_uchar(*(oInputImg.data+idx_img),*(m_voBGImg[nSampleIdx].data+idx_img))<=nCurrColorDistThreshold)
						nGoodSamplesCount++;
				}
				nSampleIdx++;
			}
			if(nGoodSamplesCount<m_nRequiredBGSamples)
				oFGMask.at<uchar>(m_voKeyPoints[k].pt) = UCHAR_MAX;
			else {
				if((rand()%nLearningRate)==0) {
					int s_rand = rand()%m_nBGSamples;
					LBSP::computeSingle(oInputImg,cv::Mat(),x,y,nCurrColorDistThreshold,nCurrInputDesc);
					m_voBGDesc[s_rand].at<unsigned short>(y,x) = nCurrInputDesc;
					m_voBGImg[s_rand].at<uchar>(y,x) = oInputImg.at<uchar>(y,x);
				}
				if((rand()%nLearningRate)==0) {
					int s_rand = rand()%m_nBGSamples;
					int x_rand,y_rand;
					getRandNeighborPosition(x_rand,y_rand,x,y,LBSP::PATCH_SIZE/2,m_oImgSize);
					LBSP::computeSingle(oInputImg,cv::Mat(),x,y,nCurrColorDistThreshold,nCurrInputDesc);
					m_voBGDesc[s_rand].at<unsigned short>(y_rand,x_rand) = nCurrInputDesc;
					m_voBGImg[s_rand].at<uchar>(y_rand,x_rand) = oInputImg.at<uchar>(y,x);
				}
			}
		}
	}
	else { //m_nImgChannels==3
		const int nCurrDescDistThreshold = m_nDescDistThreshold*3;
		const int nCurrColorDistThreshold = m_nColorDistThreshold*3;
		const int nCurrSCDescDistThreshold = m_nDescDistThreshold*BGSLBSP_SINGLECHANNEL_THRESHOLD_DIFF_FACTOR;
		const int nCurrSCColorDistThreshold = m_nColorDistThreshold*BGSLBSP_SINGLECHANNEL_THRESHOLD_DIFF_FACTOR;
		unsigned short anCurrInputDesc[3];
		int anDescDist[3], anColorDist[3];
		const int desc_row_step = m_voBGDesc[0].step.p[0];
		const int img_row_step = m_voBGImg[0].step.p[0];
		for(int k=0; k<nKeyPoints; ++k) {
			const int x = (int)m_voKeyPoints[k].pt.x;
			const int y = (int)m_voKeyPoints[k].pt.y;
			const int idx_desc = desc_row_step*y + 6*x;
			const int idx_img = img_row_step*y + 3*x;
			int nGoodSamplesCount=0, nSampleIdx=0;
			while(nGoodSamplesCount<m_nRequiredBGSamples && nSampleIdx<m_nBGSamples) {
				LBSP::computeSingle(oInputImg,m_voBGImg[nSampleIdx],x,y,m_nColorDistThreshold,anCurrInputDesc);
				const unsigned short* bgdesc_ptr = (unsigned short*)(m_voBGDesc[nSampleIdx].data+idx_desc);
				const uchar* inputimg_ptr = oInputImg.data+idx_img;
				const uchar* bgimg_ptr = m_voBGImg[nSampleIdx].data+idx_img;
				for(int n=0;n<3; ++n) {
					anDescDist[n] = hdist_ushort_8bitLUT(anCurrInputDesc[n],bgdesc_ptr[n]);
					if(anDescDist[n]>nCurrSCDescDistThreshold)
						goto skip;
					anColorDist[n] = absdiff_uchar(inputimg_ptr[n],bgimg_ptr[n]);
					if(anColorDist[n]>nCurrSCColorDistThreshold)
						goto skip;
				}
				if(anDescDist[0]+anDescDist[1]+anDescDist[2]<=nCurrDescDistThreshold && anColorDist[0]+anColorDist[1]+anColorDist[2]<=nCurrColorDistThreshold)
					nGoodSamplesCount++;
				skip:
				nSampleIdx++;
			}
			if(nGoodSamplesCount<m_nRequiredBGSamples)
				oFGMask.at<uchar>(m_voKeyPoints[k].pt) = UCHAR_MAX;
			else {
				if((rand()%nLearningRate)==0) {
					int s_rand = rand()%m_nBGSamples;
					unsigned short* bgdesc_ptr = ((unsigned short*)(m_voBGDesc[s_rand].data + desc_row_step*y + 6*x));
					LBSP::computeSingle(oInputImg,cv::Mat(),x,y,m_nColorDistThreshold,bgdesc_ptr);
					const int img_row_step = m_voBGImg[0].step.p[0];
					for(int n=0; n<3; ++n)
						*(m_voBGImg[s_rand].data + img_row_step*y + 3*x + n) = *(oInputImg.data + img_row_step*y + 3*x + n);
					CV_DbgAssert(m_voBGImg[s_rand].at<cv::Vec3b>(y,x)==oInputImg.at<cv::Vec3b>(y,x));
				}
				if((rand()%nLearningRate)==0) {
					int s_rand = rand()%m_nBGSamples;
					int x_rand,y_rand;
					getRandNeighborPosition(x_rand,y_rand,x,y,LBSP::PATCH_SIZE/2,m_oImgSize);
					unsigned short* bgdesc_ptr = ((unsigned short*)(m_voBGDesc[s_rand].data + desc_row_step*y_rand + 6*x_rand));
					LBSP::computeSingle(oInputImg,cv::Mat(),x,y,m_nColorDistThreshold,bgdesc_ptr);
					const int img_row_step = m_voBGImg[0].step.p[0];
					for(int n=0; n<3; ++n)
						*(m_voBGImg[s_rand].data + img_row_step*y_rand + 3*x_rand + n) = *(oInputImg.data + img_row_step*y + 3*x + n);
					CV_DbgAssert(m_voBGImg[s_rand].at<cv::Vec3b>(y_rand,x_rand)==oInputImg.at<cv::Vec3b>(y,x));
				}
			}
		}
	}
	cv::medianBlur(oFGMask,oFGMask,9);
}

cv::AlgorithmInfo* BackgroundSubtractorLBSP::info() const {
	CV_Assert(false); // NOT IMPL @@@@@
	return NULL;
}

cv::Mat BackgroundSubtractorLBSP::getCurrentBGImage() const {
	return m_voBGImg[0].clone();
}

cv::Mat BackgroundSubtractorLBSP::getCurrentBGDescriptors() const {
	return m_voBGDesc[0].clone();
}

std::vector<cv::KeyPoint> BackgroundSubtractorLBSP::getBGKeyPoints() const {
	return m_voKeyPoints;
}

void BackgroundSubtractorLBSP::setBGKeyPoints(std::vector<cv::KeyPoint>& keypoints) {
	m_oExtractor.validateKeyPoints(keypoints,m_oImgSize);
	CV_Assert(!keypoints.empty());
	// @@@@ NOT IMPL
	CV_Assert(false);
	// need to reinit sample buffers...
}