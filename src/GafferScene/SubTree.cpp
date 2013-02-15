//////////////////////////////////////////////////////////////////////////
//  
//  Copyright (c) 2012, John Haddon. All rights reserved.
//  Copyright (c) 2013, Image Engine Design Inc. All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//  
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//  
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//  
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  
//////////////////////////////////////////////////////////////////////////

#include <set>

#include "boost/tokenizer.hpp"

#include "Gaffer/Context.h"

#include "GafferScene/SubTree.h"

using namespace std;
using namespace Gaffer;
using namespace GafferScene;

IE_CORE_DEFINERUNTIMETYPED( SubTree );

size_t SubTree::g_firstPlugIndex = 0;

SubTree::SubTree( const std::string &name )
	:	SceneProcessor( name )
{
	storeIndexOfNextChild( g_firstPlugIndex );
	addChild( new StringPlug( "root", Plug::In, "" ) );
}

SubTree::~SubTree()
{
}

Gaffer::StringPlug *SubTree::rootPlug()
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

const Gaffer::StringPlug *SubTree::rootPlug() const
{
	return getChild<StringPlug>( g_firstPlugIndex );
}

void SubTree::affects( const ValuePlug *input, AffectedPlugsContainer &outputs ) const
{
	SceneProcessor::affects( input, outputs );
	
	if( input == rootPlug() )
	{
		outputs.push_back( outPlug() );
	}
}

void SubTree::hash( const Gaffer::ValuePlug *output, const Gaffer::Context *context, IECore::MurmurHash &h ) const
{	
	SceneProcessor::hash( output, context, h );

	if( output->parent<ScenePlug>() == outPlug() )
	{
		if( output == outPlug()->globalsPlug() )
		{
			inPlug()->globalsPlug()->hash( h );
			rootPlug()->hash( h );
		}
		else
		{
			const ScenePath &path = context->get<ScenePath>( ScenePlug::scenePathContextName );
			ScenePath source = sourcePath( path );
			
			if( output == outPlug()->boundPlug() )
			{
				h = inPlug()->boundHash( source );
			}
			else if( output == outPlug()->transformPlug() )
			{
				/// \todo If there were virtual hash*() methods in SceneNode
				/// then we wouldn't need to do this check.
				if( path.size() )
				{
					h = inPlug()->transformHash( source );
				}
			}
			else if( output == outPlug()->attributesPlug() )
			{
				if( path.size() )
				{
					h = inPlug()->attributesHash( source );
				}
			}
			else if( output == outPlug()->objectPlug() )
			{
				if( path.size() )
				{
					h = inPlug()->objectHash( source );
				}
			}
			else if( output == outPlug()->childNamesPlug() )
			{
				h = inPlug()->childNamesHash( source );
			}
		}
	}
}

Imath::Box3f SubTree::computeBound( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	return inPlug()->bound( sourcePath( path ) );
}

Imath::M44f SubTree::computeTransform( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	return inPlug()->transform( sourcePath( path ) );
}

IECore::ConstCompoundObjectPtr SubTree::computeAttributes( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	return inPlug()->attributes( sourcePath( path ) );
}

IECore::ConstObjectPtr SubTree::computeObject( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{
	return inPlug()->object( sourcePath( path ) );
}

IECore::ConstInternedStringVectorDataPtr SubTree::computeChildNames( const ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const
{		
	return inPlug()->childNames( sourcePath( path ) );
}

IECore::ConstCompoundObjectPtr SubTree::computeGlobals( const Gaffer::Context *context, const ScenePlug *parent ) const
{
	IECore::CompoundObjectPtr result = inPlug()->globalsPlug()->getValue()->copy();

	const IECore::CompoundData *inputForwardDeclarations = result->member<IECore::CompoundData>( "gaffer:forwardDeclarations" );
	if( inputForwardDeclarations )
	{
		std::string root = rootPlug()->getValue();
		IECore::CompoundDataPtr forwardDeclarations = new IECore::CompoundData;
		for( IECore::CompoundDataMap::const_iterator it = inputForwardDeclarations->readable().begin(), eIt = inputForwardDeclarations->readable().end(); it != eIt; it++ )
		{
			const IECore::InternedString &inputPath = it->first;
			std::string outputPath( inputPath, root.size() );
			forwardDeclarations->writable()[outputPath] = it->second;
		}
		result->members()["gaffer:forwardDeclarations"] = forwardDeclarations;
	}

	return result;
}

SceneNode::ScenePath SubTree::sourcePath( const ScenePath &outputPath ) const
{
	typedef boost::tokenizer<boost::char_separator<char> > Tokenizer;
	/// \todo We should introduce a plug type which stores its values as a ScenePath directly.
	string rootAsString = rootPlug()->getValue();
	Tokenizer rootTokenizer( rootAsString, boost::char_separator<char>( "/" ) );	
	ScenePath result;
	for( Tokenizer::const_iterator it = rootTokenizer.begin(), eIt = rootTokenizer.end(); it != eIt; it++ )
	{
		result.push_back( *it );
	}
	result.insert( result.end(), outputPath.begin(), outputPath.end() );
	return result;
}
