// ================================================================================
// == This file is a part of Tinkerbell UI Toolkit. (C) 2011-2012, Emil Seger�s ==
// ==                   See tinkerbell.h for more information.                   ==
// ================================================================================

#include "addons/tbimage/tb_image_manager.h"
#include "tb_system.h"

namespace tinkerbell {

// == TBImageManagerAddonFactory ========================================================

class TBImageManagerAddonFactory : public TBAddonFactory
{
public:
	virtual TBAddon *CreateAddon()
	{
		return new TBImageManager();
	}
};
TBImageManagerAddonFactory image_manager_addon_factory;

// == TBImageRep ========================================================================

TBImageRep::TBImageRep(TBImageManager *image_manager, TBBitmapFragment *fragment, uint32 hash_key)
	: ref_count(0), hash_key(hash_key), image_manager(image_manager), fragment(fragment)
{
}

void TBImageRep::IncRef()
{
	ref_count++;
}

void TBImageRep::DecRef()
{
	ref_count--;
	if (ref_count == 0)
	{
		if (image_manager)
			image_manager->RemoveImageRep(this);
		delete this;
	}
}

// == TBImage ===========================================================================

TBImage::TBImage(TBImageRep *rep)
	: m_image_rep(rep)
{
	if (m_image_rep)
		m_image_rep->IncRef();
}

TBImage::TBImage(const TBImage &image)
	: m_image_rep(image.m_image_rep)
{
	if (m_image_rep)
		m_image_rep->IncRef();
}

TBImage::~TBImage()
{
	if (m_image_rep)
		m_image_rep->DecRef();
}

bool TBImage::IsEmpty() const
{
	return m_image_rep && m_image_rep->fragment;
}

int TBImage::Width() const
{
	if (m_image_rep && m_image_rep->fragment)
		return m_image_rep->fragment->Width();
	return 0;
}

int TBImage::Height() const
{
	if (m_image_rep && m_image_rep->fragment)
		return m_image_rep->fragment->Height();
	return 0;
}

TBBitmapFragment *TBImage::GetBitmap() const
{
	return m_image_rep ? m_image_rep->fragment : nullptr;
}

void TBImage::SetImageRep(TBImageRep *image_rep)
{
	if (m_image_rep == image_rep)
		return;

	if (m_image_rep)
		m_image_rep->DecRef();

	m_image_rep = image_rep;

	if (m_image_rep)
		m_image_rep->IncRef();
}

// == TBImageManager ====================================================================

TBImageManager *g_image_manager = nullptr;

bool TBImageManager::Init()
{
	g_image_manager = this;
	g_renderer->AddListener(this);
	return true;
}

void TBImageManager::Shutdown()
{
	g_renderer->RemoveListener(this);

	// If there is TBImageRep objects live, we must unset the fragment pointer
	// since the m_frag_manager is going to be destroyed very soon.
	TBHashTableIteratorOf<TBImageRep> it(&m_image_rep_hash);
	while (TBImageRep *image_rep = it.GetNextContent())
	{
		image_rep->fragment = nullptr;
		image_rep->image_manager = nullptr;
	}
	g_image_manager = nullptr;
}

TBImage TBImageManager::GetImage(const char *filename)
{
	uint32 hash_key = TBGetHash(filename);
	TBImageRep *image_rep = m_image_rep_hash.Get(hash_key);
	if (!image_rep)
	{
		TBBitmapFragment *fragment = m_frag_manager.GetFragmentFromFile(filename, false);
		image_rep = new TBImageRep(this, fragment, hash_key);
		if (!image_rep || !fragment || !m_image_rep_hash.Add(hash_key, image_rep))
		{
			delete image_rep;
			m_frag_manager.FreeFragment(fragment);
			image_rep = nullptr;
		}
		TBDebugOut(image_rep ? "TBImageManager - Loaded new image.\n" : "TBImageManager - Loading image failed.\n");
	}
	return TBImage(image_rep);
}

void TBImageManager::RemoveImageRep(TBImageRep *image_rep)
{
	assert(image_rep->ref_count == 0);
	if (image_rep->fragment)
	{
		m_frag_manager.FreeFragment(image_rep->fragment);
		image_rep->fragment = nullptr;
	}
	m_image_rep_hash.Remove(image_rep->hash_key);
	image_rep->image_manager = nullptr;
	TBDebugOut("TBImageManager - Removed image.\n");
}

void TBImageManager::OnContextLost()
{
	m_frag_manager.DeleteBitmaps();
}

void TBImageManager::OnContextRestored()
{
	// No need to do anything. The bitmaps will be created when drawing.
}

}; // namespace tinkerbell