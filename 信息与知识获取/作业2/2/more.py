import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin
import time
import os
import json
import re

base_url = "https://www.bbc.co.uk/news/world"
headers = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"
}

def get_article_links(page_url):
    """获取页面上的新闻文章链接"""
    response = requests.get(page_url, headers=headers)
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # 使用能够找到链接的选择器
    news_links = soup.select('a[href*="/news/"]')
    
    links = []
    for a in news_links:
        href = a.get('href')
        if href and '/news/' in href:
            # 排除不需要的页面类型
            if any(x in href for x in ['/topics/', '/tags/', '/in_pictures/', '/world/']):
                continue
            # 确保链接是完整的URL
            if href.startswith('/'):
                full_url = urljoin("https://www.bbc.co.uk", href)
                links.append(full_url)
            else:
                links.append(href)
    
    # 尝试查找下一页链接
    next_page = soup.find('a', string=lambda text: text and 'Next' in text)
    next_url = urljoin(base_url, next_page['href']) if next_page else None
    
    return list(set(links)), next_url  # 使用set去除重复链接

def get_article_content(article_url):
    """获取文章的标题和内容"""
    response = requests.get(article_url, headers=headers)
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # 获取文章标题
    title_elem = (
        soup.find(['h1', 'h2'], class_=lambda c: c and ('StyledHeading' in c or 'story-headline' in c)) or 
        soup.find(['h1', 'h2']) or 
        soup.find('title')
    )
    title = title_elem.text.strip() if title_elem else "无标题"
    
    # 获取文章内容
    content_blocks = (
        soup.select('article p, div[data-component="text-block"], div.ssrcss-11r1m41-RichTextComponentWrapper p') or
        soup.select('article p, [data-component="text-block"]') or
        soup.select('.story-body p, .story-body__inner p')
    )
    content = '\n\n'.join([p.text.strip() for p in content_blocks if p.text.strip()])
    
    # 检测是否为视频内容
    is_video = False
    if (
        '/av/' in article_url or 
        soup.select('[data-testid="mediaplayer"]') or 
        soup.select('video') or
        "This video can not be played" in content or
        "This video cannot be played" in content
    ):
        is_video = True
    
    return {
        'title': title,
        'url': article_url,
        'content': content,
        'is_video': is_video
    }

def save_article(article_data, output_dir='bbc_articles'):
    """保存文章到本地文件"""
    # 如果内容为空，则不保存
    if not article_data['content'].strip():
        print(f"跳过保存空内容: {article_data['title']}")
        return None
    
    # 文章保存到articles目录
    article_dir = os.path.join(output_dir, 'articles')
    
    # 创建保存目录
    if not os.path.exists(article_dir):
        os.makedirs(article_dir)
    
    # 使用标题作为文件名，处理不适合作为文件名的字符
    filename = re.sub(r'[\\/*?:"<>|]', "_", article_data['title'])
    filename = filename.strip()[:100]  # 限制文件名长度
    
    filepath = os.path.join(article_dir, f"{filename}.txt")
    
    # 检查文件是否已存在（避免重复）
    if os.path.exists(filepath):
        print(f"文件已存在，跳过: {filepath}")
        return None
    
    # 保存文章内容、标题和链接
    with open(filepath, 'w', encoding='utf-8') as f:
        f.write(f"标题: {article_data['title']}\n\n")
        f.write(f"链接: {article_data['url']}\n\n")
        f.write(f"内容类型: 文章\n\n")
        f.write(f"内容:\n{article_data['content']}")
    
    return filepath

def update_article_index(new_articles, output_dir='bbc_articles'):
    """更新文章索引，添加新文章"""
    index_path = os.path.join(output_dir, 'articles_index.txt')
    json_path = os.path.join(output_dir, 'index.json')
    
    # 读取现有索引
    existing_articles = []
    if os.path.exists(json_path):
        with open(json_path, 'r', encoding='utf-8') as f:
            try:
                existing_articles = json.load(f)
            except:
                pass
    
    # 合并新旧文章列表
    all_articles = existing_articles + new_articles
    
    # 保存更新后的JSON索引
    with open(json_path, 'w', encoding='utf-8') as f:
        json.dump(all_articles, f, ensure_ascii=False, indent=2)
    
    # 更新文章索引文本文件
    articles_only = [a for a in all_articles if not a.get('is_video', False)]
    with open(index_path, 'w', encoding='utf-8') as f:
        f.write(f"BBC文章索引 (共{len(articles_only)}篇)\n\n")
        for i, article in enumerate(articles_only, 1):
            f.write(f"{i}. {article['title']}\n   {article['url']}\n\n")
    
    return len(articles_only)

def get_existing_urls(output_dir='bbc_articles'):
    """获取已经爬取的URL列表，避免重复爬取"""
    json_path = os.path.join(output_dir, 'index.json')
    existing_urls = set()
    
    if os.path.exists(json_path):
        with open(json_path, 'r', encoding='utf-8') as f:
            try:
                articles = json.load(f)
                for article in articles:
                    existing_urls.add(article['url'])
            except:
                pass
    
    return existing_urls

def crawl_additional_articles(more_articles=35, output_dir='bbc_articles'):
    """爬取额外的纯文章内容（不包括视频）"""
    print(f"开始爬取额外{more_articles}篇BBC世界新闻文章...")
    
    # 读取现有的URL，避免重复爬取
    existing_urls = get_existing_urls(output_dir)
    print(f"已有{len(existing_urls)}篇文章/视频，将避免重复爬取")
    
    category_urls = [
        "https://www.bbc.co.uk/news/world/asia",
        "https://www.bbc.co.uk/news/world/europe", 
        "https://www.bbc.co.uk/news/world/us_and_canada",
        "https://www.bbc.co.uk/news/world/latin_america",
        "https://www.bbc.co.uk/news/world/africa",
        "https://www.bbc.co.uk/news/world/middle_east",
        "https://www.bbc.co.uk/news/world/australia",
        # 添加更多分类以增加文章来源
        "https://www.bbc.co.uk/news/business",
        "https://www.bbc.co.uk/news/technology",
        "https://www.bbc.co.uk/news/science_and_environment",
        "https://www.bbc.co.uk/news/health"
    ]
    
    new_articles_data = []
    articles_found = 0
    processed_links = set(existing_urls)  # 初始化为已存在的链接
    
    # 遍历所有分类页面
    for category_url in category_urls:
        if articles_found >= more_articles:
            break
            
        current_url = category_url
        while current_url and articles_found < more_articles:
            print(f"正在爬取页面：{current_url}")
            links, next_url = get_article_links(current_url)
            
            # 处理获取到的链接
            for link in links:
                if articles_found >= more_articles:
                    break
                    
                if link in processed_links:
                    continue
                
                processed_links.add(link)
                
                try:
                    print(f"正在处理文章: {link}")
                    article_data = get_article_content(link)
                    
                    # 跳过视频内容
                    if article_data.get('is_video', False):
                        print(f"跳过视频内容: {article_data['title']}")
                        continue
                        
                    # 跳过空内容
                    if not article_data['content'].strip():
                        print(f"跳过空内容: {article_data['title']}")
                        continue
                    
                    # 保存文章
                    file_path = save_article(article_data, output_dir)
                    if file_path:  # 如果成功保存
                        print(f"已保存到: {file_path}")
                        new_articles_data.append(article_data)
                        articles_found += 1
                        print(f"已找到{articles_found}/{more_articles}篇文章")
                    
                    time.sleep(1)  # 防止请求过快
                except Exception as e:
                    print(f"处理文章时出错: {link}\n错误信息: {e}")
            
            current_url = next_url
            time.sleep(2)  # 翻页间隔
    
    # 更新索引
    if new_articles_data:
        total_articles = update_article_index(new_articles_data, output_dir)
        print(f"文章索引已更新，现在共有{total_articles}篇文章")
    
    print(f"成功添加了{len(new_articles_data)}篇新文章到{output_dir}目录")
    return new_articles_data

if __name__ == "__main__":
    crawl_additional_articles(35)