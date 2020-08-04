
function adjustDate(date, offset)
{
    var lNow = new Date();
    var lServerOffset = offset !== undefined ? offset : 180; // server time offset
    var lLocalOffset = lNow.getTimezoneOffset();

    var lUTCDate = new Date(date.getTime() - lServerOffset*60000);
    var lLocalDate = new Date(lUTCDate.getTime() - lLocalOffset*60000);

    return lLocalDate;
}

function toUniString(date, format)
{
    var lDay = "" + date.getDate();
    if (lDay.length == 1) lDay = "0" + lDay;

    var lMonth = "" + (date.getMonth()+1);
    if (lMonth.length == 1) lMonth = "0" + lMonth;

    var lHours = "" + date.getHours();
    if (lHours.length == 1) lHours = "0" + lHours;

    var lMinutes = "" + date.getMinutes();
    if (lMinutes.length == 1) lMinutes = "0" + lMinutes;

    var lSeconds = "" + date.getSeconds();
    if (lSeconds.length == 1) lSeconds = "0" + lSeconds;

    var lString = "";

    if (!format || format === 0)
    {
        lString = lDay + "/" + lMonth + "/" + date.getFullYear() + " " + lHours + ":" +
            lMinutes + ":" + lSeconds;
    }
    else if (format === 1)
    {
        lString = lDay + "/" + lMonth + " " + lHours + ":" +
            lMinutes;
    }
    else if (format === 2)
    {
        lString = lDay + "/" + lMonth + " " + lHours + ":" +
            lMinutes + ":" + lSeconds;
    }

    return lString;
}

function extractDateString(raw_date, offset)
{
    if (raw_date === null || raw_date === undefined || raw_date === "") return "";
    return toUniString(adjustDate(new Date(raw_date), offset));
}

function extractShortString(raw_date, offset)
{
    if (raw_date === null || raw_date === undefined || raw_date === "") return "";
    return toUniString(adjustDate(new Date(raw_date), offset), 1);
}

function extractFullTimeString(raw_date, offset)
{
    if (raw_date === null || raw_date === undefined || raw_date === "") return "";
    return toUniString(adjustDate(new Date(raw_date), offset), 2);
}
