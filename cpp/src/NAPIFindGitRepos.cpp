#include <iostream>

#include <napi.h>

enum class FindGitReposParam { NONE, STARTING_PATH, PROGRESS_CALLBACK };

class FindGitReposWorker : public Napi::AsyncWorker {
public:
  FindGitReposWorker(Napi::Env env, std::string starting_path,
                     int32_t throttle_timeout,
                     Napi::ThreadSafeFunction progress_callback)
      : Napi::AsyncWorker(env), deferred_(env),
        starting_path_(std::move(starting_path)),
        throttle_timeout_(throttle_timeout),
        progress_callback_(std::move(progress_callback)) {}
  FindGitReposWorker(Napi::Env env, FindGitReposParam invalid_param)
      : Napi::AsyncWorker(env), deferred_(env), invalid_param_(invalid_param) {}
  ~FindGitReposWorker() {
    if (invalid_param_ == FindGitReposParam::NONE) {
      progress_callback_.Release();
    }
  }

  void Execute() {
    if (invalid_param_ == FindGitReposParam::STARTING_PATH) {
      SetError("Must provide starting path as first argument.");
      return;
    }
    if (invalid_param_ == FindGitReposParam::PROGRESS_CALLBACK) {
      SetError("Must provide progress callback as second argument.");
      return;
    }

    auto progress_fn = [](Napi::Env env, Napi::Function jsCallback,
                          int *value) {
      jsCallback.Call({Napi::Number::New(env, *value)});
      delete value;
    };

    int *value = new int{321};
    if (progress_callback_.BlockingCall(value, progress_fn) != napi_ok) {
      SetError("Progress Callback failed");
      return;
    }
  }

  void OnOK() override { deferred_.Resolve(Napi::String::New(Env(), "OK")); }

  void OnError(const Napi::Error &e) override {
    deferred_.Reject(Napi::String::New(Env(), e.Message()));
  }

  Napi::Promise get_promise() { return deferred_.Promise(); }

private:
  Napi::Promise::Deferred deferred_;
  std::string starting_path_;
  int32_t throttle_timeout_;
  Napi::ThreadSafeFunction progress_callback_;
  const FindGitReposParam invalid_param_ = FindGitReposParam::NONE;
};

Napi::Value Method(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  FindGitReposParam invalid_param = FindGitReposParam::NONE;

  std::string starting_path;
  if (info.Length() >= 1 && info[0].IsString()) {
    starting_path = info[0].As<Napi::String>().Utf8Value();
  } else {
    invalid_param = FindGitReposParam::STARTING_PATH;
  }

  Napi::ThreadSafeFunction progress_callback;
  if (invalid_param == FindGitReposParam::NONE) {
    if (info.Length() >= 2 && info[1].IsFunction()) {
      progress_callback = Napi::ThreadSafeFunction::New(
          env, info[2].As<Napi::Function>(), "Progress Callback", 0, 1);
    } else {
      invalid_param = FindGitReposParam::PROGRESS_CALLBACK;
    }
  }

  int32_t throttle_timeout = 0;
  if (invalid_param == FindGitReposParam::NONE && info.Length() >= 3 &&
      info[2].IsObject()) {
    std::cerr << "im here" << std::endl;
    auto object = info[2].As<Napi::Object>();
    auto timeout = object.Get("throttleTimeoutMS");
    if (timeout.IsNumber()) {
      std::cout << "isnum\n";
      throttle_timeout = timeout.As<Napi::Number>().Int32Value();
    }
  }

  std::cerr << "invalid_param: " << static_cast<int>(invalid_param)
            << std::endl;

  auto worker =
      invalid_param == FindGitReposParam::NONE
          ? new FindGitReposWorker(env, starting_path, throttle_timeout,
                                   progress_callback)
          : new FindGitReposWorker(env, invalid_param);
  worker->Queue();
  return worker->get_promise();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "findGitRepos"),
              Napi::Function::New(env, Method));
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)