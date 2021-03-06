#pragma once
#include "pack.h"
#include "map"
#include "string"

using std::map;
using std::string;

typedef unsigned termcount;


static inline void
	read_wdf(const char ** posptr, const char * end, termcount * wdf_ptr)
{
	unpack_uint(posptr, end, wdf_ptr);
}

static inline void
	read_did_increase(const char ** posptr, const char * end,
	docid * did_ptr)
{
	docid did_increase;
	unpack_uint(posptr, end, &did_increase);
	*did_ptr += did_increase + 1;
}

class PostlistChunkReader {
    string data;

    const char *pos;
    const char *end;

    bool at_end;

    docid did;
    termcount wdf;

  public:
    /** Initialise the postlist chunk reader.
     *
     *  @param first_did  First document id in this chunk.
     *  @param data       The tag string with the header removed.
     */
    PostlistChunkReader(const string & data_)
	: data(data_), pos(data.data()), end(pos + data.length()), at_end(data.empty())
    {
		if(!at_end)
		{
			unpack_uint( &pos, end, &did );
			next();
		}
		
    }

    docid get_docid() const {
	return did;
    }
    termcount get_wdf() const {
	return wdf;
    }

    bool is_at_end() const {
	return at_end;
    }

    /** Advance to the next entry.  Set at_end if we run off the end.
     */
    void next();
};

void PostlistChunkReader::next()
{
	if (pos == end) {
		at_end = true;
	} else {
		read_did_increase(&pos, end, &did);
		read_wdf(&pos, end, &wdf);
	}
}

class PostlistChunkWriter {
    public:
	PostlistChunkWriter( string& chunk_)
		: chunk(chunk_)
	{
		started = false;
	};

	/// Append an entry to this chunk.
	void append(docid did,
		    termcount wdf);

	/// Append a block of raw entries to this chunk.
	void raw_append(docid first_did_, docid current_did_,
			const string & s) {
	    first_did = first_did_;
	    current_did = current_did_;
	    if (!s.empty()) {
		chunk.append(s);
		started = true;
	    }
	}

	/** Flush the chunk to the buffered table.  Note: this may write it
	 *  with a different key to the original one, if for example the first
	 *  entry has changed.
	 */

    private:
	string orig_key;
	string tname;
	bool is_first_chunk;
	bool is_last_chunk;
	bool started;

	docid first_did;
	docid current_did;

public:
	string& chunk;
};


void PostlistChunkWriter::append(docid did,
	termcount wdf)
{
	if (!started) {
		started = true;
		first_did = did;
		pack_uint( chunk, first_did-1 );
		pack_uint( chunk, 0 );
		started = true;
	} else {
		// Start a new chunk if this one has grown to the threshold.
		//if (chunk.size() >= CHUNKSIZE) {
		//	bool save_is_last_chunk = is_last_chunk;
		//	is_last_chunk = false;
		//	flush(table);
		//	is_last_chunk = save_is_last_chunk;
		//	is_first_chunk = false;
		//	first_did = did;
		//	chunk.resize(0);
		//	orig_key = BrassPostListTable::make_key(tname, first_did);
		//} else {
			pack_uint(chunk, did - current_did - 1);
		//}
	}
	current_did = did;
	pack_uint(chunk, wdf);
}


string merge_doclen_changes(const map<docid, doclen> & doclens, const string& chunk )
{

	if (doclens.empty()) return string();
	map<docid, termcount>::const_iterator j;
	j = doclens.begin();

	PostlistChunkReader *from = new PostlistChunkReader(chunk);

	string new_chunk;
	PostlistChunkWriter *to = new PostlistChunkWriter(new_chunk);
	for ( ; j != doclens.end(); ++j) {
		docid did = j->first;

		if (from) while (!from->is_at_end()) {
			docid copy_did = from->get_docid();
			if (copy_did >= did) {
				if (copy_did == did) from->next();
				break;
			}
			to->append(copy_did, from->get_wdf());
			from->next();
		}

		termcount new_doclen = j->second;
		if (new_doclen != static_cast<termcount>(-1)) {
			to->append(did, new_doclen);
		}
	}

	if (from) {
		while (!from->is_at_end()) {
			to->append(from->get_docid(), from->get_wdf());
			from->next();
		}
		delete from;
	}
	delete to;
	return new_chunk;
}

class PostlistChunk
{
public:
	docid did;
	termcount wdf;
	const char *pos, *end;
	string chunk;
	PostlistChunk( const string& chunk_) : chunk(chunk_)
	{
		did = 0;
		wdf = -1;
		pos = chunk.data();
		end = pos+chunk.size();
		read_wdf( &pos, end, &did );
	}
	termcount get_doc_length( docid desired_did )
	{
		if ( move_forward_in_chunk_to_at_least(desired_did ))
		{
			return wdf;
		}
		return -1;
		
	}
	bool move_forward_in_chunk_to_at_least(docid desired_did  )
	{
		if (did >= desired_did)
		{
			pos = chunk.data();
			end = pos+chunk.size();
			read_wdf( &pos, end, &did );
		}

		//if (desired_did <= last_did_in_chunk) {
			while (pos != end) {
				read_did_increase(&pos, end, &did);
				if (did >= desired_did) {
					read_wdf(&pos, end, &wdf);
					return true;
				}
				// It's faster to just skip over the wdf than to decode it.
				read_wdf(&pos, end, NULL);
			}

		//	// If we hit the end of the chunk then last_did_in_chunk must be wrong.
		//	Assert(false);
		//}

		//pos = end;
		return false;
	}
};


