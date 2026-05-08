import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Terminal, Shield, Code, CheckCircle2 } from "lucide-react";
import { Button } from "@/components/ui/button";
import { toast } from "sonner";

export default function Docs() {
  const baseUrl = window.location.origin;

  const copyCode = (text: string) => {
    navigator.clipboard.writeText(text);
    toast.success("Code copied to clipboard");
  };

  return (
    <div className="space-y-8 max-w-4xl">
      <div>
        <h1 className="text-3xl font-bold font-mono tracking-tight mb-2">Integration Documentation</h1>
        <p className="text-muted-foreground text-lg">
          API reference for validating licenses from your software DLLs.
        </p>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-8">
        <Card className="bg-card border-border/50 shadow-sm">
          <CardHeader className="pb-2">
            <Shield className="w-6 h-6 text-primary mb-2" />
            <CardTitle className="text-lg">Secure Verification</CardTitle>
          </CardHeader>
          <CardContent className="text-sm text-muted-foreground">
            Keys are checked against real-time revocation status and expiration limits.
          </CardContent>
        </Card>
        <Card className="bg-card border-border/50 shadow-sm">
          <CardHeader className="pb-2">
            <Terminal className="w-6 h-6 text-primary mb-2" />
            <CardTitle className="text-lg">Simple REST API</CardTitle>
          </CardHeader>
          <CardContent className="text-sm text-muted-foreground">
            A single POST endpoint is all you need to implement in your client code.
          </CardContent>
        </Card>
        <Card className="bg-card border-border/50 shadow-sm">
          <CardHeader className="pb-2">
            <CheckCircle2 className="w-6 h-6 text-primary mb-2" />
            <CardTitle className="text-lg">JSON Responses</CardTitle>
          </CardHeader>
          <CardContent className="text-sm text-muted-foreground">
            Clear, unambiguous validation states and remaining time metadata.
          </CardContent>
        </Card>
      </div>

      <div className="space-y-6">
        <Card className="bg-card border-border/50 shadow-sm overflow-hidden">
          <div className="bg-secondary/50 px-6 py-4 border-b border-border/50 flex items-center justify-between">
            <div className="flex items-center space-x-3">
              <span className="px-2 py-1 bg-primary/20 text-primary font-mono text-xs font-bold rounded">POST</span>
              <span className="font-mono text-sm tracking-tight">{baseUrl}/api/keys/validate</span>
            </div>
          </div>
          <CardContent className="p-6 space-y-6">
            <div>
              <h3 className="text-sm font-semibold text-foreground mb-3 flex items-center uppercase tracking-wider">
                <Code className="w-4 h-4 mr-2 text-primary" /> Request Body
              </h3>
              <div className="relative group">
                <pre className="bg-background/80 p-4 rounded-md border border-border/50 overflow-x-auto text-sm font-mono text-muted-foreground">
{`{
  "key": "XIT-ABCD-EFGH-IJKL-MNOP"
}`}
                </pre>
                <Button 
                  size="sm" 
                  variant="secondary" 
                  className="absolute top-2 right-2 opacity-0 group-hover:opacity-100 transition-opacity h-7 text-xs"
                  onClick={() => copyCode(`{\n  "key": "XIT-ABCD-EFGH-IJKL-MNOP"\n}`)}
                >
                  Copy
                </Button>
              </div>
            </div>

            <div>
              <h3 className="text-sm font-semibold text-foreground mb-3 flex items-center uppercase tracking-wider">
                <Code className="w-4 h-4 mr-2 text-primary" /> Success Response (200 OK)
              </h3>
              <div className="relative group">
                <pre className="bg-background/80 p-4 rounded-md border border-border/50 overflow-x-auto text-sm font-mono text-muted-foreground">
{`{
  "valid": true,
  "expiresAt": "2024-05-15T12:00:00Z",
  "timeRemaining": "7 days",
  "message": "License is valid"
}`}
                </pre>
                <Button 
                  size="sm" 
                  variant="secondary" 
                  className="absolute top-2 right-2 opacity-0 group-hover:opacity-100 transition-opacity h-7 text-xs"
                  onClick={() => copyCode(`{\n  "valid": true,\n  "expiresAt": "2024-05-15T12:00:00Z",\n  "timeRemaining": "7 days",\n  "message": "License is valid"\n}`)}
                >
                  Copy
                </Button>
              </div>
            </div>

            <div>
              <h3 className="text-sm font-semibold text-foreground mb-3 flex items-center uppercase tracking-wider">
                <Code className="w-4 h-4 mr-2 text-primary" /> Failed Response (200 OK)
              </h3>
              <p className="text-sm text-muted-foreground mb-2">Note: Failed validation still returns 200 OK but with <code>valid: false</code>.</p>
              <div className="relative group">
                <pre className="bg-background/80 p-4 rounded-md border border-border/50 overflow-x-auto text-sm font-mono text-muted-foreground">
{`{
  "valid": false,
  "expiresAt": null,
  "timeRemaining": null,
  "message": "Key is revoked" // or "Key has expired", "Invalid key format", "Key not found"
}`}
                </pre>
              </div>
            </div>
          </CardContent>
        </Card>
        
        <Card className="bg-card border-border/50 shadow-sm">
          <CardHeader>
            <CardTitle>C++ Implementation Example</CardTitle>
            <CardDescription>A simple example of calling the validation endpoint from a C++ DLL using cURL.</CardDescription>
          </CardHeader>
          <CardContent>
            <div className="relative group">
              <pre className="bg-background/80 p-4 rounded-md border border-border/50 overflow-x-auto text-xs font-mono text-muted-foreground leading-relaxed">
{`#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool ValidateLicense(const std::string& key) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = "${baseUrl}/api/keys/validate";
    json payload = {{"key", key}};
    std::string data = payload.dump();
    
    // ... setup curl options, headers (Content-Type: application/json)
    // ... execute request and parse JSON response
    
    // bool isValid = response_json["valid"].get<bool>();
    // return isValid;
    return true; 
}`}
              </pre>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  );
}
