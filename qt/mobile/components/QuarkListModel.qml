// QuarkListModel.qml

import QtQuick 2.0

ListModel
{
    id: listModel

    property string sortBy: "name";
    property string sortBy2: "";
    property string sortOrder: "asc";

    function swap(a, b)
    {
        if (a < b)
        {
            move(a, b, 1);
            move(b-1, a, 1);
        }
        else if (a > b)
        {
            move(b, a, 1);
            move(a-1, b, 1);
        }
    }

    function partition(begin, end, pivot, criteria)
    {
        var piv = get(pivot)[criteria];
        var piv2 = null;
        if (sortBy2.length) piv2 = get(pivot)[sortBy2];

        swap(pivot, end-1);

        var store = begin;
        var ix;

        for(ix=begin; ix<end-1; ++ix)
        {
            var lItem = get(ix);

            if (sortOrder === "asc")
            {
                if(lItem[criteria] < piv)
                {
                    swap(store, ix);
                    ++store;
                }
                else if(lItem[criteria] === piv)
                {
                    if (piv2 && lItem[sortBy2] < piv2)
                    {
                        swap(store, ix);
                        ++store;
                    }
                }
            }
            else if (sortOrder === "desc")
            {
                if(lItem[criteria] > piv)
                {
                    swap(store, ix);
                    ++store;
                }
                else if(lItem[criteria] === piv)
                {
                    if (piv2 && lItem[sortBy2] > piv2)
                    {
                        swap(store, ix);
                        ++store;
                    }
                }
            }
        }

        swap(end-1, store);

        return store;
    }

    function qsort(begin, end, criteria)
    {
        if(end-1 > begin)
        {
            var pivot = begin + Math.floor(Math.random() * (end - begin));

            pivot = partition(begin, end, pivot, criteria);

            qsort(begin, pivot, criteria);
            qsort(pivot+1, end, criteria);
        }
    }

    function sort()
    {
        qsort(0, count, sortBy);
    }

    function estimate(candidate)
    {
        var lIndex = -1;

        for(var lIdx = 0; lIdx < count; lIdx++)
        {
            var lItem = get(lIdx);

            if (sortOrder === "asc")
            {
                if(lItem[sortBy] > candidate[sortBy])
                {
                    lIndex = lIdx; break;
                }
                else if (lItem[sortBy] === candidate[sortBy])
                {
                    if (sortBy2.length && lItem[sortBy2] > candidate[sortBy2]) { lIndex = lIdx; break; }
                }
            }
            else if (sortOrder === "desc")
            {
                if(lItem[sortBy] < candidate[sortBy])
                {
                    lIndex = lIdx;
                    break;
                }
                else if (lItem[sortBy] === candidate[sortBy])
                {
                    if (sortBy2.length && lItem[sortBy2] < candidate[sortBy2]) { lIndex = lIdx; break; }
                }
            }
        }

        return lIndex;
    }

    function push(candidate)
    {
        var lIndex = estimate(candidate);
        if (lIndex === -1) lIndex = 0;
        insert(lIndex, candidate);
    }

    function reverseSortOrder()
    {
        if (sortOrder === "asc") sortOrder = "desc";
        else sortOrder = "asc";

        return sortOrder;
    }
}

