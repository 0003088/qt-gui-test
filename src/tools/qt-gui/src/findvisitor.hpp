/**
 * @file
 *
 * @brief
 *
 * @copyright BSD License (see doc/LICENSE.md or http://www.libelektra.org)
 */

#ifndef FINDVISITOR_HPP
#define FINDVISITOR_HPP

#include "confignode.hpp"
#include "visitor.hpp"

/**
 * @brief The FindVisitor class. It performs the search for a term and includes all ConfigNodes that contain the search
 * term in their name, value or metadata.
 */
class FindVisitor : public Visitor
{
public:
	/**
	 * @brief FindVisitor The constructor.
	 * @param searchResults An empty TreeViewModel that will contain the ConfigNode s that match the searchterm after the search
	 * is completed.
	 * @param term The search term to look for.
	 */
	explicit FindVisitor (TreeViewModel * searchResults, QString term);

	void visit (TreeItem & item) override;

private:
	TreeViewModel * m_searchResults;
	QString m_term;
};

#endif // FINDVISITOR_HPP
