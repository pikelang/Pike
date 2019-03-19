#pike __REAL_VERSION__

//! Internal class ment to be inherited by other Google API's
class Api
{
  inherit Web.Api.Api : parent;

  // Just a convenience class
  protected class Method
  {
    inherit Web.Api.Api.Method;

    //! Internal convenience method
    protected mixed _get(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::get(get_uri(METHOD_PATH + s), p, cb);
    }

    //! Internal convenience method
    protected mixed _post(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::post(get_uri(METHOD_PATH + s), p, 0, cb);
    }

    //! Internal convenience method
    protected mixed _delete(string s, void|ParamsArg p, void|Callback cb)
    {
      return parent::delete(get_uri(METHOD_PATH + s), p, cb);
    }
  }
}
